#include "artdaq-demo/Generators/ToyHardwareInterface/ToyHardwareInterface.hh"
#define TRACE_NAME "ToyHardwareInterface"
#include "artdaq-core-demo/Overlays/FragmentType.hh"
#include "artdaq-core-demo/Overlays/ToyFragment.hh"
#include "artdaq/DAQdata/Globals.hh"

#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <random>

// JCF, Mar-17-2016

// ToyHardwareInterface is meant to mimic a vendor-provided hardware
// API, usable within the the ToySimulator fragment generator. For
// purposes of realism, it's a C++03-style API, as opposed to, say, one
// based in C++11 capable of taking advantage of smart pointers, etc.

ToyHardwareInterface::ToyHardwareInterface(fhicl::ParameterSet const& ps)
    : taking_data_(false)
    , nADCcounts_(ps.get<size_t>("nADCcounts", 40))
    , maxADCcounts_(ps.get<size_t>("maxADCcounts", 50000000))
    , change_after_N_seconds_(ps.get<size_t>("change_after_N_seconds", std::numeric_limits<size_t>::max()))
    , pause_after_N_seconds_(ps.get<size_t>("pause_after_N_seconds", 0))
    , nADCcounts_after_N_seconds_(ps.get<size_t>("nADCcounts_after_N_seconds", nADCcounts_))
    , exception_after_N_seconds_(ps.get<bool>("exception_after_N_seconds", false))
    , exit_after_N_seconds_(ps.get<bool>("exit_after_N_seconds", false))
    , abort_after_N_seconds_(ps.get<bool>("abort_after_N_seconds", false))
    , hang_after_N_seconds_(ps.get<bool>("hang_after_N_seconds", false))
    , fragment_type_(demo::toFragmentType(ps.get<std::string>("fragment_type")))
    , maxADCvalue_(static_cast<size_t>(pow(2, NumADCBits()) - 1))
    ,  // MUST be after "fragment_type"
    throttle_usecs_(ps.get<size_t>("throttle_usecs", 100000))
    , usecs_between_sends_(ps.get<size_t>("usecs_between_sends", 0))
    , distribution_type_(static_cast<DistributionType>(ps.get<int>("distribution_type")))
    , engine_(ps.get<int64_t>("random_seed", 314159))
    , uniform_distn_(new std::uniform_int_distribution<data_t>(0, maxADCvalue_))
    , gaussian_distn_(new std::normal_distribution<double>(0.5 * maxADCvalue_, 0.1 * maxADCvalue_))
    , start_time_(fake_time_)
    , send_calls_(0)
    , serial_number_((*uniform_distn_)(engine_))
{
	if (nADCcounts_ > maxADCcounts_ ||
	    nADCcounts_after_N_seconds_ > maxADCcounts_)
	{
		throw cet::exception("HardwareInterface")  // NOLINT(cert-err60-cpp)
		    << R"(Either (or both) of "nADCcounts" and "nADCcounts_after_N_seconds")"
		    << " is larger than the \"maxADCcounts\" setting (currently at " << maxADCcounts_ << ")";
	}

	bool planned_disruption = nADCcounts_after_N_seconds_ != nADCcounts_ || exception_after_N_seconds_ ||
	                          exit_after_N_seconds_ || abort_after_N_seconds_;

	if (planned_disruption && change_after_N_seconds_ == std::numeric_limits<size_t>::max())
	{
		throw cet::exception("HardwareInterface") << "A FHiCL parameter designed to create a disruption has been "  // NOLINT(cert-err60-cpp)
		                                             "set, so \"change_after_N_seconds\" should be set as well";
	}
}

// JCF, Mar-18-2017

// "StartDatataking" is meant to mimic actions one would take when
// telling the hardware to start sending data - the uploading of
// values to registers, etc.

void ToyHardwareInterface::StartDatataking()
{
	taking_data_ = true;
	send_calls_ = 0;
}

void ToyHardwareInterface::StopDatataking()
{
	taking_data_ = false;
	start_time_ = fake_time_;
}

void ToyHardwareInterface::FillBuffer(char* buffer, size_t* bytes_read)
{
	TLOG(TLVL_TRACE) << "FillBuffer BEGIN";
	if (taking_data_)
	{
		TLOG(TLVL_DEBUG + 3) << "FillBuffer: Sleeping for " << throttle_usecs_ << " microseconds";
		usleep(throttle_usecs_);

		auto elapsed_secs_since_datataking_start = artdaq::TimeUtils::GetElapsedTime(start_time_);
		if (elapsed_secs_since_datataking_start < 0) elapsed_secs_since_datataking_start = 0;

		if (static_cast<size_t>(elapsed_secs_since_datataking_start) < change_after_N_seconds_ || send_calls_ == 0)
		{
			TLOG(TLVL_DEBUG + 3) << "FillBuffer: Setting bytes_read to " << sizeof(demo::ToyFragment::Header) + nADCcounts_ * sizeof(data_t);
			*bytes_read = sizeof(demo::ToyFragment::Header) + nADCcounts_ * sizeof(data_t);
		}
		else
		{
			if (abort_after_N_seconds_)
			{
				TLOG(TLVL_ERROR) << "Engineered Abort!";
				std::abort();
			}
			else if (exit_after_N_seconds_)
			{
				TLOG(TLVL_ERROR) << "Engineered Exit!";
				std::exit(1);
			}
			else if (exception_after_N_seconds_)
			{
				TLOG(TLVL_ERROR) << "Engineered Exception!";
				throw cet::exception("HardwareInterface")  // NOLINT(cert-err60-cpp)
				    << "This is an engineered exception designed for testing purposes";
			}
			else if (hang_after_N_seconds_)
			{
				TLOG(TLVL_ERROR) << "Pretending that the hardware has hung! Variable name for gdb: hardwareIsHung";
				volatile bool hardwareIsHung = true;
				// Pretend the hardware hangs
				while (hardwareIsHung)
				{
					usleep(10000);
				}
			}
			else
			{
				if ((pause_after_N_seconds_ != 0u) && (static_cast<size_t>(elapsed_secs_since_datataking_start) % change_after_N_seconds_ == 0))
				{
					TLOG(TLVL_DEBUG + 3) << "pausing " << pause_after_N_seconds_ << " seconds";
					sleep(pause_after_N_seconds_);
					TLOG(TLVL_DEBUG + 3) << "resuming after pause of " << pause_after_N_seconds_ << " seconds";
				}
				TLOG(TLVL_DEBUG + 3) << "FillBuffer: Setting bytes_read to " << sizeof(demo::ToyFragment::Header) + nADCcounts_after_N_seconds_ * sizeof(data_t);
				*bytes_read = sizeof(demo::ToyFragment::Header) + nADCcounts_after_N_seconds_ * sizeof(data_t);
			}
		}

		TLOG(TLVL_DEBUG + 3) << "FillBuffer: Making the fake data, starting with the header";

		// Can't handle a fragment whose size isn't evenly divisible by
		// the demo::ToyFragment::Header::data_t type size in bytes
		// std::cout << "Bytes to read: " << *bytes_read << ", sizeof(data_t): " <<
		// sizeof(demo::ToyFragment::Header::data_t) << std::endl;
		assert(*bytes_read % sizeof(demo::ToyFragment::Header::data_t) == 0);

		auto* header = reinterpret_cast<demo::ToyFragment::Header*>(buffer);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

		header->event_size = *bytes_read / sizeof(demo::ToyFragment::Header::data_t);
		header->trigger_number = 99;
		header->distribution_type = static_cast<uint8_t>(distribution_type_);

		TLOG(TLVL_DEBUG + 3) << "FillBuffer: Generating nADCcounts ADC values ranging from 0 to max based on the desired distribution";

		std::function<data_t()> generator;
		data_t gen_seed = 0;

		switch (distribution_type_)
		{
			case DistributionType::uniform:
				generator = [&]() { return static_cast<data_t>((*uniform_distn_)(engine_)); };
				break;

			case DistributionType::gaussian:
				generator = [&]() {
					do
					{
						gen_seed = static_cast<data_t>(std::round((*gaussian_distn_)(engine_)));
					} while (gen_seed > maxADCvalue_);
					return gen_seed;
				};
				break;

			case DistributionType::monotonic: {
				generator = [&]() {
					if (++gen_seed > maxADCvalue_)
					{
						gen_seed = 0;
					}
					return gen_seed;
				};
			}
			break;

			case DistributionType::uninitialized:
			case DistributionType::uninit2:
				break;

			default:
				throw cet::exception("HardwareInterface") << "Unknown distribution type specified";  // NOLINT(cert-err60-cpp)
		}

		if (distribution_type_ != DistributionType::uninitialized && distribution_type_ != DistributionType::uninit2)
		{
			TLOG(TLVL_DEBUG + 3) << "FillBuffer: Calling generate_n";
			std::generate_n(reinterpret_cast<data_t*>(reinterpret_cast<demo::ToyFragment::Header*>(buffer) + 1),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
			                nADCcounts_, generator);
		}
	}
	else
	{
		throw cet::exception("ToyHardwareInterface") << "Attempt to call FillBuffer when not sending data";  // NOLINT(cert-err60-cpp)
	}

	if (send_calls_ == 0)
	{
		TLOG(TLVL_DEBUG + 3) << "FillBuffer has set the start_time_";
		start_time_ = std::chrono::steady_clock::now();
	}

	if (usecs_between_sends_ != 0)
	{
		if (send_calls_ > 0)
		{
			auto usecs_since_start = artdaq::TimeUtils::GetElapsedTimeMicroseconds(start_time_);
			double delta = static_cast<double>(usecs_between_sends_ * send_calls_) - usecs_since_start;
			TLOG(TLVL_DEBUG + 3) << "FillBuffer send_calls=" << send_calls_ << " usecs_since_start=" << usecs_since_start
			                     << " delta=" << delta;
			if (delta > 0)
			{
				TLOG(TLVL_DEBUG + 3) << "FillBuffer: Sleeping for " << delta << " microseconds";
				usleep(delta);
			}
		}
	}
	++send_calls_;
	TLOG(TLVL_TRACE) << "FillBuffer END";
}

void ToyHardwareInterface::AllocateReadoutBuffer(char** buffer)
{
	*buffer = reinterpret_cast<char*>(  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	    new uint8_t[sizeof(demo::ToyFragment::Header) + maxADCcounts_ * sizeof(data_t)]);
}

void ToyHardwareInterface::FreeReadoutBuffer(const char* buffer) { delete[] buffer; }

int ToyHardwareInterface::BoardType() const
{
	// Pretend that the "BoardType" is some vendor-defined integer which
	// differs from the fragment_type_ we want to use as developers (and
	// which must be between 1 and 224, inclusive) so add an offset
	return static_cast<int>(fragment_type_) + 1000;
}

int ToyHardwareInterface::NumADCBits() const
{
	switch (fragment_type_)
	{
		case demo::FragmentType::TOY1:
			return 12;
			break;
		case demo::FragmentType::TOY2:
			return 14;
			break;
		default:
			throw cet::exception("ToyHardwareInterface") << "Unknown board type " << fragment_type_ << " ("  // NOLINT(cert-err60-cpp)
			                                             << demo::fragmentTypeToString(fragment_type_) << ").\n";
	};
}

int ToyHardwareInterface::SerialNumber() const
{
	// Serial number is generated from the uniform distribution on initialization of the class
	return serial_number_;
}
