#pragma once

#include <map>
#include <vector>
#include <string>

#include "systemc.h"

#include "../config.h"
#include "primitive.h"

namespace prims
{
	/// @brief The Load primitive.
	class PrimMatrixMultiply :
		public Primitive
	{
	public:
		PrimMatrixMultiply(
			uint64_t left_addr,
			uint64_t left_h,
			uint64_t left_w,
			uint64_t right_addr,
			uint64_t right_w,
			uint64_t bias_addr,
			uint64_t output_addr
		);
		PrimMatrixMultiply(sc_bv<MEM_PORT_WIDTH> prim_id_code);

		void convertParams2PrimIDCode();

	public:
		// prims params

		uint64_t left_addr_;
		uint64_t left_h_;
		uint64_t left_w_;
		uint64_t right_addr_;
		uint64_t right_w_;
		uint64_t bias_addr_;
		uint64_t output_addr_;
	};
}

