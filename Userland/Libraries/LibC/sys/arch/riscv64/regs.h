/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Platform.h>

#if defined(__cplusplus) && defined(__cpp_concepts)
#    include <AK/Types.h>
#else
#    include <sys/types.h>
#endif

#include <Kernel/Arch/mcontext.h>

#ifdef __cplusplus
struct [[gnu::packed]] PtraceRegisters : public __mcontext {
#    if defined(__cplusplus) && defined(__cpp_concepts)
    FlatPtr ip() const
    {
        return pc;
    }

    void set_ip(FlatPtr ip)
    {
        pc = ip;
    }

    FlatPtr bp() const
    {
        return 0; // TODO
        // return x[29];
    }

    void set_bp([[maybe_unused]] FlatPtr bp)
    {
        // TODO
        // x[29] = bp;
    }
#    endif
};

#else
typedef struct __mcontext PthreadRegisters;
#endif
