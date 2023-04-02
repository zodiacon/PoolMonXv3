#include "pch.h"
#include "Helpers.h"
#include <fstream>

std::wstring Helpers::ExtractResource(UINT id, PCWSTR type, PCWSTR filename) {
    auto hRes = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(id), type);
    if (!hRes)
        return {};

    auto size = ::SizeofResource(_Module.GetResourceInstance(), hRes);
    if (size == 0)
        return {};

    auto hGlobal = ::LoadResource(_Module.GetResourceInstance(), hRes);
    if (!hGlobal)
        return {};

    auto p = ::LockResource(hGlobal);
    WCHAR path[MAX_PATH];
    if (0 == ::GetTempPath(_countof(path), path))
        return {};

    if (0 == GetTempFileName(path, filename, 0, path))
        return {};

    std::ofstream stm;
    stm.open(path, std::ios_base::out);
    if (!stm.good())
        return {};

    stm.write((const char*)p, size);

    return path;
}
