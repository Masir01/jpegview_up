#pragma once

// For usage in std::unordered_map with LPCTSTR keys
struct CHashLPCTSTR {
	size_t operator()(const LPCTSTR& Key) const;
};

struct CEqualLPCTSTR {
	bool operator()(const LPCTSTR& _Key1, const LPCTSTR& _Key2) const;
};
