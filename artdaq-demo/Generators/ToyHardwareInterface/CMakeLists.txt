cet_make(
        LIB_LIBRARIES
        art_Utilities
        fhiclcpp
        cetlib
        cetlib_except
  MF_MessageLogger
  artdaq_DAQdata
	artdaq_core::artdaq-core_Data
	artdaq_core_demo::artdaq-core-demo_Overlays
	${Boost_SYSTEM_LIBRARY}
        )
        
install_headers()
install_source()