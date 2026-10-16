//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Modules.hpp"
#include "Environment.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Module {
	std::string path;
	fs::file_time_type mtime;
	P<Scope> scope;  // kept across reloads, so functions of an old version find the new one's names
	P<Namespace> ns;
	int64_t serial = 0;  // which load this is, across all modules
	std::vector<std::pair<std::string, int64_t>> deps;  // the modules it imported, at their serials then
};

std::map<std::string, Module> gModules;
int64_t gSerial = 0;
std::vector<std::string> gFileDirs;  // directories of the files being loaded, innermost last
std::vector<std::string> gLoading;   // modules being loaded, for cycles
std::vector<std::vector<std::pair<std::string, int64_t>>*> gDeps;  // where each load records its imports

template <class T>
struct Pushed {
	std::vector<T>& v;
	Pushed(std::vector<T>& inV, T const& x) : v(inV) { v.push_back(x); }
	~Pushed() { v.pop_back(); }
};

std::string findModule(const std::string& name)
{
	std::vector<std::string> dirs = {gFileDirs.empty() ? fs::current_path().string() : gFileDirs.back()};
	if (const char* path = stacEnv("PATH")) {
		std::stringstream ss(path);
		for (std::string dir; std::getline(ss, dir, ':');)
			if (!dir.empty()) dirs.push_back(dir);
	}
	std::error_code ec;
	for (std::string const& dir : dirs) {
		fs::path p = fs::path(dir) / (name + ".stac");
		if (fs::is_regular_file(p, ec)) return fs::absolute(p, ec).lexically_normal().string();
	}
	return "";
}

bool identical(Arg a, Arg b) { return a.o() == b.o() && (a.o() || a.f == b.f); }

// Runs the file in a fresh scope, or the module's own scope when it reloads, and makes its
// namespace from the names it defined: not those its imports bound.
void load(const std::string& name, const std::string& path, P<Scope> scope)
{
	std::ifstream f(path, std::ios::binary);
	std::stringstream src;
	if (!f || !(src << f.rdbuf())) syntaxError(("import : cannot read '" + path + "'").c_str());
	Pushed<std::string> loading(gLoading, name);
	LoadingFile file(path);
	std::vector<std::pair<std::string, int64_t>> deps;
	Pushed<std::vector<std::pair<std::string, int64_t>>*> recording(gDeps, &deps);

	post("loading module %s from '%s'\n", name.c_str(), path.c_str());
	Thread mt;
	mt.mScope = scope;
	mt.mWorkspace = new GForm();
	P<Fun> fun;
	std::string text = src.str();
	if (!mt.compile(text.c_str(), fun, true)) syntaxError(("import : module " + name + " does not compile").c_str());
	fun->run(mt);
	mt.publishWorkspace();

	P<Namespace> ns = new Namespace(name.c_str());
	for (GForm* g = mt.mWorkspace(); g; g = g->mNextForm())
		for (P<TreeNode> const& node : g->mTable->sorted()) {
			if (!node->mKey.isString()) continue;
			std::string key = ((String*)node->mKey.o())->s;
			auto imported = mt.mImported.find(key);
			if (ns->mWords.count(key) || (imported != mt.mImported.end() && identical(imported->second, node->mValue)))
				continue;
			ns->mWords[key] = node->mValue;
		}
	std::error_code ec;
	gModules[name] = {path, fs::last_write_time(path, ec), scope, ns, ++gSerial, deps};
}

bool upToDate(Thread& th, Module& m)
{
	std::error_code ec;
	if (fs::last_write_time(m.path, ec) != m.mtime) return false;
	LoadingFile file(m.path);
	std::vector<std::pair<std::string, int64_t>> ignored;
	Pushed<std::vector<std::pair<std::string, int64_t>>*> recording(gDeps, &ignored);
	for (auto& [dep, serial] : m.deps) {
		importModule(th, dep);  // reloads it if it changed
		auto it = gModules.find(dep);
		if (it == gModules.end() || it->second.serial != serial) return false;
	}
	return true;
}

} // namespace

LoadingFile::LoadingFile(const std::string& path)
{
	std::error_code ec;
	gFileDirs.push_back(fs::absolute(fs::path(path), ec).parent_path().string());
}

LoadingFile::~LoadingFile() { gFileDirs.pop_back(); }

Namespace* importModule(Thread& th, const std::string& name)
{
	std::string path = findModule(name);
	if (path.empty()) return nullptr;
	auto cycle = std::find(gLoading.begin(), gLoading.end(), name);
	if (cycle != gLoading.end()) {
		std::string chain;
		for (auto it = cycle; it != gLoading.end(); ++it) chain += *it + " -> ";
		syntaxError(("import cycle: " + chain + name).c_str());
	}
	auto it = gModules.find(name);
	if (it == gModules.end() || it->second.path != path || !upToDate(th, it->second)) {
		P<Scope> scope = it != gModules.end() && it->second.path == path ? it->second.scope : P<Scope>(new Scope(new GForm()));
		load(name, path, scope);
	}
	Module& m = gModules[name];
	if (!gDeps.empty()) gDeps.back()->push_back({name, m.serial});
	return m.ns();
}
