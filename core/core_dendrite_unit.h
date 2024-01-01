#pragma once
#include "systemc.h"
#include "../components/ram.h"
#include "../components/dram.h"
#include "../prims/primitive.h"
#include "../components/functional_noc.h"
#include <vector>
#include "../utils/BFloat16.h"


namespace core {
	enum dendrite_unit_status
	{
		DENDRITE_IDLE = 0, DENDRITE_PRIM
	};



	class DendriteUnit : public sc_module
	{
	public:
		SC_HAS_PROCESS(DendriteUnit);
		DendriteUnit(const sc_module_name& name, Event_engine* _e_engine) :
			sc_module(name), e_engine_(_e_engine)
		{
			SC_THREAD(dendriteStateTrans);
			sensitive << rst.pos() << dendrite_prim_finish.posedge_event() << dendrite_prim_start.pos();
			dont_initialize();

			SC_THREAD(dendritePrimExecution);
			sensitive << dendrite_status << rst.pos();
			dont_initialize();
		}

		void dendriteStateTrans();
		void dendritePrimExecution();
		vector<vector<float>> readMatrix(uint64_t addr, uint64_t height, uint64_t width);
		void writeMatrix(uint64_t addr, vector<vector<float>> matrix, uint64_t height, uint64_t width);

		vector<vector<vector<float>>> readTensor(uint64_t addr, uint64_t n_channels, uint64_t height, uint64_t width);
		void writeTensor(vector<vector<vector<float>>> tensor, uint64_t addr, uint64_t n_channels, uint64_t height, uint64_t width);

	public:
		sc_in<sc_bv<MEM_PORT_WIDTH> > prim_in;

		sc_in<bool> rst;
		sc_in<bool> dendrite_prim_start;
		sc_out<bool> dendrite_busy;

		sc_port<ram_if<sc_bv<MEM_PORT_WIDTH> > > core_mem_port;

		sc_signal<bool> dendrite_prim_finish;
		sc_buffer<dendrite_unit_status> dendrite_status;

		Event_engine* e_engine_;

	};
}
