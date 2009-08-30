/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef RADEON_COMPILER_H
#define RADEON_COMPILER_H

#include "memory_pool.h"
#include "radeon_code.h"
#include "radeon_program.h"


struct radeon_compiler {
	struct memory_pool Pool;
	struct rc_program Program;
	unsigned Debug:1;
	unsigned Error:1;
	char * ErrorMsg;
};

void rc_init(struct radeon_compiler * c);
void rc_destroy(struct radeon_compiler * c);

void rc_debug(struct radeon_compiler * c, const char * fmt, ...);
void rc_error(struct radeon_compiler * c, const char * fmt, ...);

void rc_calculate_inputs_outputs(struct radeon_compiler * c);

void rc_move_input(struct radeon_compiler * c, unsigned input, struct rc_src_register new_input);
void rc_move_output(struct radeon_compiler * c, unsigned output, unsigned new_output, unsigned writemask);
void rc_copy_output(struct radeon_compiler * c, unsigned output, unsigned dup_output);
void rc_transform_fragment_wpos(struct radeon_compiler * c, unsigned wpos, unsigned new_input);

struct r300_fragment_program_compiler {
	struct radeon_compiler Base;
	struct rX00_fragment_program_code *code;
	struct r300_fragment_program_external_state state;
	unsigned is_r500;
	unsigned OutputDepth;
	unsigned OutputColor;

	void * UserData;
	void (*AllocateHwInputs)(
		struct r300_fragment_program_compiler * c,
		void (*allocate)(void * data, unsigned input, unsigned hwreg),
		void * mydata);
};

void r3xx_compile_fragment_program(struct r300_fragment_program_compiler* c);


struct r300_vertex_program_compiler {
	struct radeon_compiler Base;
	struct r300_vertex_program_code *code;
	uint32_t RequiredOutputs;

	void * UserData;
	void (*SetHwInputOutput)(struct r300_vertex_program_compiler * c);
};

void r3xx_compile_vertex_program(struct r300_vertex_program_compiler* c);

#endif /* RADEON_COMPILER_H */
