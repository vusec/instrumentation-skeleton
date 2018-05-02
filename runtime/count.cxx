#include <map>
#include <cstring>
#include <iostream>
#include <iomanip>
#include "noinstrument.h"

using namespace std;

namespace {
    static map<const char*, unsigned> counters;

    extern "C" __attribute__((nothrow))
    void NOINSTRUMENT(count_libcall)(const char *funcname) {
        counters[funcname]++;
    }

    __attribute__((destructor))
    static void NOINSTRUMENT(print_libcall_counters)() {
        unsigned w = 0;

        for (auto &it : counters) {
            const char *name = it.first;
            unsigned len = strlen(name);
            if (len > w)
                w = len;
        }

        cerr << "libcall counts:" << endl;

        for (auto &it : counters) {
            const char *name = it.first;
            const unsigned count = it.second;
            cerr << "  " << left << setw(w) << name << " : " << count << endl;
        }

        cerr.flush();
    }
}
