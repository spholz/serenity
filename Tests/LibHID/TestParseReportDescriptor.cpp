/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibTest/TestCase.h>

#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibHID/ReportDescriptorParser.h>

TEST_CASE(parse_basic_report_descriptor)
{
    // auto report_descriptor_file = TRY_OR_FAIL(Core::File::open("/usr/Tests/LibHID/report_descriptor.bin"sv, Core::File::OpenMode::Read));
    auto report_descriptor_file = TRY_OR_FAIL(Core::File::open("report_descriptor.bin"sv, Core::File::OpenMode::Read));
    auto report_descriptor = TRY_OR_FAIL(report_descriptor_file->read_until_eof());

    HID::ReportDescriptorParser parser { report_descriptor };
    auto parsed = TRY_OR_FAIL(parser.parse());

    auto recurse = [](auto self, Variant<HID::Field, HID::Collection> const& node) -> void {
        node.visit(
            [](HID::Field const& field) {
                switch (field.main_item_tag) {
                case HID::MainItemTag::Input:
                    dbgln("Input");
                    break;
                case HID::MainItemTag::Output:
                    dbgln("Output");
                    break;
                case HID::MainItemTag::Feature:
                    dbgln("Feature");
                    break;
                default:
                    VERIFY_NOT_REACHED();
                }
                dbg("    Usages: ");
                for (auto const usage : field.item_state_table.local.usages)
                    dbg("{:#x} ", usage);
                dbgln();
                dbgln("    Logical Minimum: {}", field.item_state_table.global.logical_minimum);
                dbgln("    Logical Maximum: {}", field.item_state_table.global.logical_maximum);
                dbgln("    Report Size: {}", field.item_state_table.global.report_size);
                dbgln("    Report Count: {}", field.item_state_table.global.report_count);
            },
            [self](HID::Collection const& collection) {
                for (auto const& child : collection.children) {
                    self(self, child);
                }
            });
    };

    for (auto const& child : parsed.children) {
        recurse(recurse, child);
    }

    FAIL("this is a test");
}
