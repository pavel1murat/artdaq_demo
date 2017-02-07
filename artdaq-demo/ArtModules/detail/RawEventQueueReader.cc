#include "artdaq-demo/ArtModules/detail/RawEventQueueReader.hh"
#include "artdaq-core-demo/Overlays/FragmentType.hh"

#include <messagefacility/MessageLogger/MessageLogger.h>

demo::detail::RawEventQueueReader::RawEventQueueReader(fhicl::ParameterSet const & ps,
	art::ProductRegistryHelper & help,
	art::SourceHelper const & pm) :
  artdaq::detail::RawEventQueueReader(ps, help, pm)
{
  mf::LogInfo("RawEventQueueReader") << "DemoInput Constructor!";
  for(auto& name : names) {
    mf::LogInfo("RawEventQueueReader") << "Adding fragment type " << name << " to fragment_type_map, and registering with the ProductRegistryHelper";
    fragment_type_map_[toFragmentType(name)] = name;
    help.reconstitutes<artdaq::Fragments, art::InEvent>(pretend_module_name, name);
  }
}