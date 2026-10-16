//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// stac's modules (docs/dev/stac.md, Proposal: modules): a file run in its own scope, whose
// definitions become a namespace that import binds.

#ifndef SAPF_MODULES_HPP
#define SAPF_MODULES_HPP

#include "Namespace.hpp"
#include <string>

// The namespace of module `name`, from name.stac in the importing file's directory or a directory
// of $STAC_PATH. Loads it on its first import, and again when its file, or a module it imports,
// has changed since. nullptr when no file is found. Throws, keeping the loaded version, when the
// file fails to compile or run.
Namespace* importModule(Thread& th, const std::string& name);

// While alive, `path` is the file being loaded: its imports search its directory first.
struct LoadingFile
{
	explicit LoadingFile(const std::string& path);
	~LoadingFile();
};

#endif
