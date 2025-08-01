#ifdef __GNUC__

#include <ext/vstring.h>

inline int stoi_compat(const std::string& __str, size_t* __idx = 0, int __base = 10)
{
    return __gnu_cxx::__stoa<long, int>(&strtol, "stoi", __str.c_str(), __idx, __base);
}

inline std::string to_string_compat(int __val)
{
    return __gnu_cxx::__to_xstring<std::string>(&vsnprintf, 4 * sizeof(int), "%d", __val);
}

#endif ///__GNUC__
