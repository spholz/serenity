set(TEST_SOURCES
    TestParseReportDescriptor.cpp
)

foreach(source IN LISTS TEST_SOURCES)
    serenity_test("${source}" LibHID LIBS LibHID LibFileSystem)
endforeach()

install(FILES report_descriptor.bin DESTINATION usr/Tests/LibHID)
