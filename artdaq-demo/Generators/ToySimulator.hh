#ifndef artdaq_demo_Generators_ToySimulator_hh
#define artdaq_demo_Generators_ToySimulator_hh

// ToySimulator is a simple type of fragment generator intended to be
// studied by new users of artdaq as an example of how to create such
// a generator in the "best practices" manner. Derived from artdaq's
// CommandableFragmentGenerator class, it can be used in a full DAQ
// simulation, generating all ADC counts with equal probability via
// the std::uniform_int_distribution class

// ToySimulator is designed to simulate values coming in from one of
// two types of digitizer boards, one called "TOY1" and the other
// called "TOY2"; the only difference between the two boards is the #
// of bits in the ADC values they send. These values are declared as
// FragmentType enum's in artdaq-demo's
// artdaq-demo/Overlays/FragmentType.hh header.

// Some C++ conventions used:

// -Append a "_" to every private member function and variable

#include "fhiclcpp/fwd.h"
#include "artdaq/DAQdata/Fragments.hh" 
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq-demo/Overlays/ToyFragment.hh"
#include "artdaq-demo/Overlays/FragmentType.hh"

#include <random>
#include <vector>
#include <atomic>

namespace demo {    

  class ToySimulator : public artdaq::CommandableFragmentGenerator {
  public:
    explicit ToySimulator(fhicl::ParameterSet const & ps);

  private:

    // The "getNext_" function is used to implement user-specific
    // functionality; it's a mandatory override of the pure virtual
    // getNext_ function declared in CommandableFragmentGenerator

    bool getNext_(artdaq::FragmentPtrs & output) override;

    // Like "getNext_", "fragmentIDs_" is a mandatory override; it
    // returns a vector of the fragment IDs an instance of this class
    // is responsible for (in the case of ToySimulator, this is just
    // the fragment_id_ variable declared in the parent
    // CommandableFragmentGenerator class)
    
    std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_() override {
      return fragment_ids_;
    }

    // FHiCL-configurable variables. Note that the C++ variable names
    // are the FHiCL variable names with a "_" appended

    std::size_t const nADCcounts_;     // ADC values per fragment per event
    FragmentType const fragment_type_; // Type of fragment (see FragmentType.hh)
    
    std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_; 

    // Members needed to generate the simulated data

    std::mt19937 engine_;
    std::unique_ptr<std::uniform_int_distribution<int>> uniform_distn_;
  };
}

#endif /* artdaq_demo_Generators_ToySimulator_hh */