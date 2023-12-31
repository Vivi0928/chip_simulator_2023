#include "core_dendrite_unit.h"
#include "../prims/prim_matrix_multiply.h"
#include "../utils/BFloat16.h"


/// @brief Exe unit status transition
/// Driven signals: dendrite_status, dendrite_busy
void core::DendriteUnit::dendriteStateTrans()
{
	while (true) {
		if (this->rst.read()) {
			this->dendrite_status.write(DENDRITE_IDLE);
			this->dendrite_busy.write(false);
		}
		else if (this->dendrite_status.read() == DENDRITE_PRIM) {
			if (this->dendrite_prim_finish.read()) {
				this->dendrite_status.write(DENDRITE_IDLE);
				this->dendrite_busy.write(false);
			}
		}
		else if (this->dendrite_status.read() == DENDRITE_IDLE) {
			if (dendrite_prim_start.read()) {
				this->dendrite_status.write(DENDRITE_PRIM);
				this->dendrite_busy.write(true);
			}
		}
		wait();
	}
}

void core::DendriteUnit::dendritePrimExecution()
{
	while (true) {
		if ((this->dendrite_status.read() == DENDRITE_IDLE) || this->rst.read()) {
			this->dendrite_prim_finish.write(false);
		}
		else if (this->dendrite_status == DENDRITE_PRIM) {
			sc_bv<MEM_PORT_WIDTH> prim_temp = prim_in.read();

			if (prim_temp.range(3, 0).to_uint() == 6) {
				/************************* MatrixMultiply *************************/
				prims::PrimMatrixMultiply prim_matrix_multiply(prim_temp);
				uint64_t left_h = prim_matrix_multiply.left_h_;
				uint64_t left_w = prim_matrix_multiply.left_w_;
				uint64_t right_h = prim_matrix_multiply.left_w_;
				uint64_t right_w = prim_matrix_multiply.right_w_;

				std::vector<std::vector<BFloat16>> left_matrix(left_h, std::vector<BFloat16>(left_w, 0));
				std::vector<std::vector<BFloat16>> right_matrix(right_h, std::vector<BFloat16>(right_w, 0));
				std::vector<std::vector<BFloat16>> bias_matrix(left_h, std::vector<BFloat16>(right_w, 0));

				// read the left matrix from SRAM
				left_matrix = readMatrix(prim_matrix_multiply.left_addr_, left_h, left_w);
				// read the right matrix from SRAM
				right_matrix = readMatrix(prim_matrix_multiply.right_addr_, right_h, right_w);

				// read the bias matrix from SRAM
				bias_matrix = readMatrix(prim_matrix_multiply.bias_addr_, left_h, right_w);

				// compute result
				cout << endl;
				std::vector<std::vector<BFloat16>> result_matrix(left_h, std::vector<BFloat16>(right_w, 0));
				for (int i = 0; i < left_h; ++i) {
					for (int j = 0; j < right_w; ++j) {
						float temp = 0;
						for (int k = 0; k < left_w; ++k) {
							temp += f32_from_bits(left_matrix[i][k].x) * f32_from_bits(right_matrix[k][j].x);
						}
						temp += f32_from_bits(bias_matrix[i][j].x);
						result_matrix[i][j].x += bits_from_f32(temp);
						cout << f32_from_bits(result_matrix[i][j].x) << " ";
					}
				}
				// write the result matrix to SRAM
				this->writeMatrix(prim_matrix_multiply.output_addr_, result_matrix, left_h, right_w);
			}
			else if (prim_temp.range(3, 0).to_uint() == 7) {
				/************************* Conv *************************/
			}
			else {
				assert(0);
			}
			this->dendrite_prim_finish.write(true);
		}
		wait();
	}
}

std::vector<std::vector<BFloat16>> core::DendriteUnit::readMatrix(uint64_t addr, uint64_t height, uint64_t width) {
	std::vector<std::vector<BFloat16>> matrix(height, std::vector<BFloat16>(width, 0));
	sc_bv<MEM_PORT_WIDTH> mem_temp;
	core_mem_port->read(addr, mem_temp);  // read data from memory
	uint64_t lines = 0;
	uint64_t index = 0;
	for (auto i = 0; i < height; i++) {
		for (auto j = 0; j < width; j++) {
			BFloat16 temp_bfloat16(mem_temp.range(16 * (index + 1) - 1, 16 * index));
			matrix[i][j] = temp_bfloat16;
			cout << f32_from_bits(matrix[i][j].x) << " ";
			index += 1;
			if (index == MEM_PORT_WIDTH / 16 - 1) {
				lines += 1;
				index = 0;
				core_mem_port->read(addr + lines, mem_temp);  // read data from memory
			}
		}
	}
	return matrix;
}

void core::DendriteUnit::writeMatrix(uint64_t addr, std::vector<std::vector<BFloat16>> matrix, uint64_t height, uint64_t width){
	// write the result matrix to SRAM
	sc_bv<MEM_PORT_WIDTH> mem_temp;
	uint64_t lines = 0;
	uint64_t index = 0;
	for (auto i = 0; i < height; i++) {
		for (auto j = 0; j < width; j++) {
			mem_temp.range(16 * (index + 1) - 1, 16 * index) = to_bits(matrix[i][j]);
			index += 1;
			if (index == MEM_PORT_WIDTH / 16 - 1) {
				core_mem_port->write(addr + lines, mem_temp, 1);
				lines += 1;
				index = 0;
			}
		}
	}

	// Write the remaining portion of the matrix into SRAM. 
	// Ensuring it is merged with the irrelavent content in SRAM.
	sc_bv<MEM_PORT_WIDTH> irrelavent_part;
	core_mem_port->read(addr + lines + 1, irrelavent_part);
	mem_temp.range(MEM_PORT_WIDTH - 1, 16 * (index + 1)) = irrelavent_part(MEM_PORT_WIDTH - 1, 16 * (index + 1));
	core_mem_port->write(addr + lines, mem_temp, 1);
}
