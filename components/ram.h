#pragma once
#ifndef _RAM_H
#define _RAM_H

#include <iostream>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "systemc.h"

#include "../config.h"
#include "mem_if.h"
#include "../trace_engine/Event_engine.h"


template<class T>
class ram : public sc_channel,public ram_if<T>
{
private:
	T* mem;
	uint64_t m_start_address;
	uint64_t m_end_address;
	int* valid;

public:
	static int random_access_times;
	static float energy_consumption;
	static int total_access_latency;
	static float area;
	Event_engine* e_engine_;
	bool ram_write_busy;
	bool ram_read_busy;

public:
	ram(sc_module_name name_, uint64_t start_address,uint64_t end_address, Event_engine* _e_engine)
		:m_start_address(start_address)
		,m_end_address(end_address) {
		//sc_assert(m_end_address >= m_start_addresss);
		mem = new T[m_end_address - m_start_address];
		valid = new int[m_end_address - m_start_address];
		for (auto i = m_start_address;  i < m_end_address; i++)
		{
			valid[i] = 0;
		}
		e_engine_ = _e_engine;

		auto bits = (m_end_address - m_start_address) * sizeof(T) * 8;
		area = 0.001 * std::pow(0.028, 2.07) * std::pow(bits, 0.9) * std::pow(2, 0.7) + 0.0048;
	}
	~ram() {
		if (mem) { delete[] mem; }
		if (valid) { delete[] valid; }
	}

	virtual void register_port(sc_port_base& port, const char* if_typename) {
		
	}

	transfer_status read(uint64_t address, T& data)
	{
		if (address<m_start_address || address>m_end_address) {
			data = 0;
			return TRANSFER_ERROR;
		}
		ram_read_busy = 1;
		data = mem[address - m_start_address];
		Trace_event_util _util;
		_util.m_bar_name = __func__;
		e_engine_->add_event(this->name(), __func__, "B", _util);
		wait(RAM_READ_LATENCY, SC_NS);

		_util.m_bar_name = __func__;
		e_engine_->add_event(this->name(), __func__, "E", _util);

		random_access_times++;
		total_access_latency += RAM_READ_LATENCY;
		energy_consumption += RAM_READ_ENERGY;
		ram_read_busy = 0;
		return TRANSFER_OK;
	}

	transfer_status write(uint64_t address, T& data, uint64_t force_write=1) {
		if (address<m_start_address || address>m_end_address) {
			return TRANSFER_ERROR;
		}
		ram_write_busy = 1;
		mem[address - m_start_address] = data;
		if (force_write == 1) {
			valid[address - m_start_address] = 1;
		}else
		{
			if (valid[address - m_start_address] == 1)
			{
				cout << "Ram valid error������" << endl;
			}else
			{
				valid[address - m_start_address] = 1;
			}
		}
		Trace_event_util _util;
		_util.m_bar_name = __func__;
		e_engine_->add_event(this->name(), __func__, "B", _util);
		wait(RAM_WRITE_LATENCY, SC_NS);
		e_engine_->add_event(this->name(), __func__, "E", _util);


		
		random_access_times++;
		total_access_latency += RAM_WRITE_LATENCY;
		energy_consumption += RAM_WRITE_ENERGY;
		int valid_tmp = 0;
		float valid_perc = 0;
		for (auto i = m_start_address; i < m_end_address; i++)
		{
			if (valid[i] == 1)
			{
				valid_tmp++;
			};
		}
		valid_perc = (float)valid_tmp / (m_end_address - m_start_address) * 100;

		std::stringstream ss_y;
		ss_y << std::setiosflags(std::ios::fixed) << std::setprecision(3) << valid_perc;
		std::string str_valid = ss_y.str();

		if (valid_perc > Mem_usage_thre)
		{
			_util.m_color = "cq_build_attempt_failed";
		}else
		{
			_util.m_color = "cq_build_attempt_runnig";
		}

		_util.m_bar_name = str_valid + "%";
		string thread_name(this->name());
		thread_name = thread_name + " Mem Usage";
		e_engine_->add_event(this->name(), thread_name, "E", _util);
		e_engine_->add_event(this->name(), thread_name, "B", _util);
		ram_write_busy = 0;
		return TRANSFER_OK;
	}


	transfer_status clear(uint64_t address, T& data) {
		if (address<m_start_address || address>m_end_address) {
			return TRANSFER_ERROR;
		}
		valid[address - m_start_address] = 0;
	return TRANSFER_OK;
	}
	bool reset() {
		for (uint64_t i = 0; i < (m_end_address - m_start_address); i++)
			mem[i] = (T)0;
		return true;
	}

	inline uint64_t start_address()const {
		return m_start_address;
	}

	inline uint64_t end_address()const {
		return m_end_address;
	}

};

template<class T>
int ram<T>::random_access_times = 0;

template<class T>
float ram<T>::area = 0;

template<class T>
float ram<T>::energy_consumption = 0;

template<class T>
int ram<T>::total_access_latency = 0;


#endif
