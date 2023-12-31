#include "prim_matrix_multiply.h"

prims::PrimMatrixMultiply::PrimMatrixMultiply(uint64_t left_addr, uint64_t left_h, uint64_t left_w, uint64_t right_addr, uint64_t right_w, uint64_t bias_addr, uint64_t output_addr)
	: left_addr_(left_addr), left_h_(left_h), left_w_(left_w), right_addr_(right_addr), right_w_(right_w), bias_addr_(bias_addr), output_addr_(output_addr)
{
	assert(left_addr < (1 << 16));
	assert(left_h < (1 << 16));
	assert(left_w < (1 << 16));
	assert(right_addr < (1 << 16));
	assert(right_w < (1 << 16));
	assert(bias_addr < (1 << 16));
	assert(output_addr < (1 << 16));

	this->prim_name_ = "MatrixMultiply";

	this->convertParams2PrimIDCode();
}

prims::PrimMatrixMultiply::PrimMatrixMultiply(sc_bv<MEM_PORT_WIDTH> prim_id_code)
	: Primitive(prim_id_code)
{
	assert(prim_id_code.range(3, 0) == sc_bv<4>(6));

	this->left_addr_ = prim_id_code.range(19, 4).to_uint();
	this->left_h_ = prim_id_code.range(35, 20).to_uint();
	this->left_w_ = prim_id_code.range(51, 36).to_uint();
	this->right_addr_ = prim_id_code.range(67, 52).to_uint();
	this->right_w_ = prim_id_code.range(83, 68).to_uint();
	this->bias_addr_ = prim_id_code.range(99, 84).to_uint();
	this->output_addr_ = prim_id_code.range(115, 100).to_uint();

	this->prim_name_ = "MatrixMultiply";
}

void prims::PrimMatrixMultiply::convertParams2PrimIDCode()
{
	this->prim_id_code_.range(3, 0) = sc_bv<4>(6);

	this->prim_id_code_.range(19, 4) = sc_bv<16>(this->left_addr_);
	this->prim_id_code_.range(35, 20) = sc_bv<16>(this->left_h_);
	this->prim_id_code_.range(51, 36) = sc_bv<16>(this->left_w_);
	this->prim_id_code_.range(67, 52) = sc_bv<16>(this->right_addr_);
	this->prim_id_code_.range(83, 68) = sc_bv<16>(this->right_w_);
	this->prim_id_code_.range(99, 84) = sc_bv<16>(this->bias_addr_);
	this->prim_id_code_.range(115, 100) = sc_bv<16>(this->output_addr_);
}
