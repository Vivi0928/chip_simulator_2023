#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <random>

#include "../core/core.h"
#include "../prims/primitive.h"
#include "../prims/prim_load.h"
#include "../prims/prim_store.h"
#include "../prims/prim_jump.h"
#include "../prims/prim_relu.h"
#include "../prims/prim_matrix_multiply.h"

#include "../trace_engine/Event_engine.h"
#include "../utils/file_compare.h"
#include "../utils/BFloat16.h"

vector<vector<BFloat16>> readMatrixFromFile(const string& filename) {
	vector<vector<BFloat16>> matrix;
	ifstream inputFile(filename);
	if (inputFile.is_open()) {
		string line;

		while (getline(inputFile, line)) {
			istringstream iss(line);
			vector<BFloat16> row;

			float value;
			while (iss >> value) {
				BFloat16 temp(0);
				temp.x = bits_from_f32(value);
				row.push_back(temp);
			}

			matrix.push_back(row);
		}

		inputFile.close();
	}
	else {
		cerr << "Unable to open file: " << filename << endl;
	}

	return matrix;
}

class core_tb : public sc_module {
public:
	SC_HAS_PROCESS(core_tb);
	core_tb(const sc_module_name& nm, string test_name, core::Core* test_core) :
		sc_module(nm), test_core(test_core), test_name_(test_name)
	{
		this->test_core->rst(reset);
		this->test_core->start(start);
		this->test_core->stop(stop);
		this->test_core->core_in_idle(core_in_idle);
		this->data_gen();
		SC_THREAD(test_cores);
	}

	void data_gen();
	void test_cores();
	void before_start();
	void after_stop();
	vector<sc_bv<MEM_PORT_WIDTH> > matrix2bits(vector<vector<BFloat16>> matrix, uint64_t height, uint64_t width);

public:
	sc_signal<bool> reset;
	sc_signal<bool> start;
	sc_signal<bool> stop;
	sc_signal<bool> core_in_idle;

	core::Core* test_core;
	vector<sc_bv<MEM_PORT_WIDTH> > instructions;
	map<uint64_t, vector<sc_bv<MEM_PORT_WIDTH> > > data_list;

	sc_signal<bool> CPU_send_signal;
	sc_signal<bool> CPU_wait_receive_signal;

	string test_name_;
};

uint64_t left_addr = 0x0003;
uint64_t left_h = 0x0004;
uint64_t left_w = 0x0003;

uint64_t right_addr = 0x0005;
uint64_t right_h = 0x0003;
uint64_t right_w = 0x0003;

uint64_t bias_addr = 0x0007;

uint64_t output_addr = 0x0009;

void core_tb::data_gen()
{
	// prepare prims
	assert(left_w == right_h);
	assert(left_addr + left_h * left_w / 8 < right_addr);
	assert(right_addr + right_h * right_w / 8 < bias_addr);
	assert(bias_addr + left_h * right_w / 8 < output_addr);
	using namespace prims;
	instructions.push_back(convertPrim2Code(
		PrimMatrixMultiply(left_addr, left_h, left_w, right_addr, right_w, bias_addr, output_addr)
	));

	// prepare data
	cout << "--------------------- original data ---------------------" << endl;
	vector<vector<BFloat16>> left_matrix = readMatrixFromFile("matrix/left_matrix.txt");
	vector<sc_bv<MEM_PORT_WIDTH> > left_matrix_data = matrix2bits(left_matrix, left_h, left_w);
	data_list.insert(pair<uint64_t, vector<sc_bv<MEM_PORT_WIDTH> > >(left_addr, left_matrix_data));


	vector<vector<BFloat16>> right_matrix = readMatrixFromFile("matrix/right_matrix.txt");
	vector<sc_bv<MEM_PORT_WIDTH> > right_matrix_data = matrix2bits(right_matrix, right_h, right_w);
	data_list.insert(pair<uint64_t, vector<sc_bv<MEM_PORT_WIDTH> > >(right_addr, right_matrix_data));


	vector<vector<BFloat16>> bias_matrix = readMatrixFromFile("matrix/bias_matrix.txt");
	vector<sc_bv<MEM_PORT_WIDTH> > bias_matrix_data = matrix2bits(bias_matrix, left_h, right_w);
	data_list.insert(pair<uint64_t, vector<sc_bv<MEM_PORT_WIDTH> > >(bias_addr, bias_matrix_data));
}

void core_tb::before_start()
{

}

void core_tb::after_stop()
{
	bool test_pass_flag = true;
	sc_bv<MEM_PORT_WIDTH> temp;

	cout << "--------------------- output data ---------------------" << endl;
	sc_bv<MEM_PORT_WIDTH> mem_temp;
	this->test_core->core_memory->read(output_addr, mem_temp);  // read data from memory
	uint64_t lines = 0;
	uint64_t index = 0;
	for (auto i = 0; i < left_h; i++) {
		for (auto j = 0; j < right_w; j++) {
			BFloat16 temp_bfloat16(mem_temp.range(16 * (index + 1) - 1, 16 * index));
			index += 1;
			cout << f32_from_bits(temp_bfloat16.x) << " ";
			if (index == MEM_PORT_WIDTH / 16 - 1) {
				lines += 1;
				index = 0;
				this->test_core->core_memory->read(output_addr + lines, mem_temp);  // read data from memory
			}
		}
		cout << endl;
	}
}

vector<sc_bv<MEM_PORT_WIDTH>> core_tb::matrix2bits(vector<vector<BFloat16>> matrix, uint64_t height, uint64_t width)
{
	vector<sc_bv<MEM_PORT_WIDTH> > matrix_data;
	sc_bv<MEM_PORT_WIDTH> mem_temp;
	uint64_t lines = 0;
	uint64_t index = 0;
	for (auto i = 0; i < height; i++) {
		for (auto j = 0; j < width; j++) {
			mem_temp.range(16 * (index + 1) - 1, 16 * index) = to_bits(matrix[i][j]);
			index += 1;
			if (index == MEM_PORT_WIDTH / 16 - 1) {
				matrix_data.push_back(mem_temp);
				lines += 1;
				index = 0;
			}
		}
	}
	matrix_data.push_back(mem_temp);
	return matrix_data;
}

void core_tb::test_cores()
{
	reset.write(false);
	wait(10, SC_NS);
	reset.write(true);
	wait(10, SC_NS);
	reset.write(false);
	wait(10, SC_NS);

	test_core->writeData(data_list);
	test_core->writePrims(0, instructions);

	before_start();

	start.write(false);
	wait(10, SC_NS);
	start.write(true);
	wait(core_in_idle.posedge_event());

	after_stop();

	// print memory usage
	cout << "--------------------- memory usage report ----------------------" << endl;
	double simulater_time = sc_time_stamp().to_double();
	std::cout << "DRAM random access times: " << dram<sc_bv<MEM_PORT_WIDTH> >::random_access_times << std::endl;
	std::cout << "DRAM burst access times: " << dram<sc_bv<MEM_PORT_WIDTH> >::burst_access_times << std::endl;
	std::cout << "DRAM area: " << dram<sc_bv<MEM_PORT_WIDTH> >::area << std::endl;
	std::cout << "DRAM refresh energy: " << dram<sc_bv<MEM_PORT_WIDTH> >::area * simulater_time * DRAM_REFRESH_POWER << std::endl;
	std::cout << "DRAM total access latency: " << dram<sc_bv<MEM_PORT_WIDTH> >::total_access_latency << std::endl;
	std::cout << "DRAM energy consumption: " << dram<sc_bv<MEM_PORT_WIDTH> >::energy_consumption << std::endl;
	std::cout << "SRAM random access times: " << ram<sc_bv<MEM_PORT_WIDTH> >::random_access_times << std::endl;
	std::cout << "SRAM area: " << ram<sc_bv<MEM_PORT_WIDTH> >::area << std::endl;
	std::cout << "SRAM static energy: " << ram<sc_bv<MEM_PORT_WIDTH> >::area * simulater_time * RAM_STATIC_POWER << std::endl;
	std::cout << "SRAM total access latency: " << ram<sc_bv<MEM_PORT_WIDTH> >::total_access_latency << std::endl;
	std::cout << "SRAM energy consumption: " << ram<sc_bv<MEM_PORT_WIDTH> >::energy_consumption << std::endl;

	sc_stop();

}


int sc_main(int argc, char* argv[])
{
	Event_engine* event_engine_test = new Event_engine("my_event_engine");
	FunctionalNoC<sc_bv<MEM_PORT_WIDTH> >* noc = new FunctionalNoC<sc_bv<MEM_PORT_WIDTH> >("noc", event_engine_test);

	core::Core test_core("test_core", CoreLoc(0, 0), noc, event_engine_test);
	core_tb host("host", "single_core_test_load_store", &test_core);

	sc_start();

	delete event_engine_test;
	delete noc;

	return 0;
}

