#ifndef _OS_LAB1_PROBEFUNCS_H
#define _OS_LAB1_PROBEFUNCS_H

#include "compfuncs.hpp"
#include <variant>
#include <iostream>
#include <cmath>

#include <chrono>
#include <thread>
#include <optional>
#include <condition_variable>


# ifdef __GNUG__
# define INLINE_CONST	constexpr
# else
# define INLINE_CONST	inline const
# endif

namespace os::lab1::compfuncs {

    using namespace std::chrono_literals;
    using duration = std::chrono::seconds;
    using std::pair;
    using std::optional;

    template<typename T>
    using case_attribs = pair<duration, optional<T>>;

    //template<typename T> comp_result<T> gen_func(optional<pair<duration, T>> attribs) {
    template<typename T> comp_result<T> gen_func(optional<case_attribs<T>> attribs) {
        if (attribs) 
        {
            std::this_thread::sleep_for(std::get<duration>(attribs.value()));
            if (std::get<1>(attribs.value()))
            {
                if (!std::get<1>(attribs.value()).value())
                    return comp_result<T>(hard_fail());
                return comp_result<T>(std::get<1>(attribs.value()).value());
            }
            else
                return comp_result<T>(soft_fail());
            
        }
        else 
        {
            std::condition_variable cv;
            std::mutex m;
            std::unique_lock<std::mutex> lock(m);
            cv.wait_for(lock, 100s, []{return false;});
            return {};
        }
    }

    template<op_group O>
    struct op_group_trial_traits;

    template<typename T> 
    struct op_group_trial_type_traits : op_group_type_traits<T> {
	typedef case_attribs<T> attr_type;
	typedef struct { optional<attr_type> f_attrs, g_attrs; } case_type;
    };

    template<> 
    struct op_group_trial_traits<INT_SUM> : op_group_trial_type_traits<int> {
    	INLINE_CONST static case_type cases[] = {
            { .f_attrs = pair(7s, 3),       .g_attrs = pair(1s, 5)       },
            { .f_attrs = pair(2s, 4),       .g_attrs = attr_type(3s, {}) },
            { .f_attrs =          {},       .g_attrs = pair(3s, 6)       },
            { .f_attrs = pair(1s, 5),       .g_attrs = pair(4s, 14)      },
            { .f_attrs = attr_type(4s, {}), .g_attrs = pair(3s, 11)      },
            { .f_attrs =          {},       .g_attrs = {}                }
        };
    };

    template<> 
    struct op_group_trial_traits<DOUBLE_MULT> : op_group_trial_type_traits<double> {
    	INLINE_CONST static case_type cases[] = {
            { .f_attrs = pair(3s, 4.),       .g_attrs = pair(7s, 1.)      },
            { .f_attrs = pair(2s, 6.),       .g_attrs = attr_type(3s, {}) },
            { .f_attrs =           {},       .g_attrs = pair(3s, 6.)      },
            { .f_attrs = pair(1s, 7.),       .g_attrs = pair(4s, 4.)      },
            { .f_attrs = attr_type(4s, {}),  .g_attrs = pair(3s, 15.)     },
            { .f_attrs =           {},       .g_attrs = {}                }
        };
    };

    template<> 
    struct op_group_trial_traits<AND> : op_group_trial_type_traits<bool> {
    	INLINE_CONST static case_type cases[] = {
            { .f_attrs = pair(2s, true),     .g_attrs = pair(5s, false)   },
            { .f_attrs = pair(3s, false),    .g_attrs = attr_type(3s, {}) },
            { .f_attrs =             {},     .g_attrs = pair(2s, true)    },
            { .f_attrs = pair(1s, true),     .g_attrs = pair(1s, true)    },
            { .f_attrs = attr_type(4s, {}),  .g_attrs = pair(3s, false)   },
            { .f_attrs =                {},  .g_attrs = {}                }
        };
    };

    template<> 
    struct op_group_trial_traits<OR> : op_group_trial_type_traits<bool> {
    	INLINE_CONST static case_type cases[] = {
            { .f_attrs = pair(2s, false),    .g_attrs = pair(5s, true)    },
            { .f_attrs = pair(3s, true),     .g_attrs = attr_type(3s, {}) },
            { .f_attrs =             {},     .g_attrs = pair(2s, false)   },
            { .f_attrs = pair(1s, false),    .g_attrs = pair(1s, false)   },
            { .f_attrs = attr_type(4s, {}),  .g_attrs = pair(3s, true)    },
            { .f_attrs =                {},  .g_attrs = {}                }
        };
    };

    template<op_group O> 
    typename op_group_traits<O>::result_type trial_f(int case_nr) {
	return gen_func<typename op_group_traits<O>::value_type>(op_group_trial_traits<O>::cases[case_nr].f_attrs);
    }

    template<op_group O> 
    typename op_group_traits<O>::result_type trial_g(int case_nr) {
	return gen_func<typename op_group_traits<O>::value_type>(op_group_trial_traits<O>::cases[case_nr].g_attrs);
    }
}

#endif // _OS_LAB1_PROBEFUNCS_H
