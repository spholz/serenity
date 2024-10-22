/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <AK/MemoryStream.h>
#include <AK/Stack.h>

#include <LibHID/ReportDescriptorDefinitions.h>

namespace HID {

// https://www.usb.org/document-library/device-class-definition-hid-111

// 5.4 Item Parser

struct ItemStateTable {
    // 6.2.2.7 Global Items
    struct {
        u16 usage_page;
        u32 logical_minimum;
        u32 logical_maximum;
        u32 physical_minimum;
        u32 physical_maximum;
        u32 unit_exponent;
        u32 unit;
        u32 report_size;
        u8 report_id;
        u32 report_count;
    } global;

    // 6.2.2.8 Local Items
    struct {
        Vector<u32, 4> usages;
        u32 usage_minimum;
        u32 usage_maximum;
        u32 designator_index;
        u32 degignator_minimum;
        u32 designator_maximum;
        u32 string_index;
        u32 string_minimum;
        u32 string_maximum;
        u32 delimiter;
    } local;
};

struct Field {
    MainItemTag main_item_tag;
    ItemStateTable item_state_table;
    size_t start_report_index;
};

struct Collection {
    Collection* parent;
    Optional<u32> usage;
    Vector<Variant<Field, Collection>> children;
};

struct ParsedReportDescriptor {
    Vector<Variant<Field, Collection>> children;
};

class ReportDescriptorParser {
public:
    ReportDescriptorParser(ReadonlyBytes);

    ErrorOr<ParsedReportDescriptor> parse();

private:
    template<typename T>
    ErrorOr<T> read_item_data(ItemHeader);

    FixedMemoryStream m_stream;
    Vector<ItemStateTable, 1> m_item_state_table_stack;
    Collection* m_current_collection { nullptr };
};

}
