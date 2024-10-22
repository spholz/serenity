/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Endian.h>
#include <LibHID/ReportDescriptorParser.h>

namespace HID {

// https://www.usb.org/document-library/device-class-definition-hid-111

ReportDescriptorParser::ReportDescriptorParser(ReadonlyBytes data)
    : m_stream(data)
{
    // The append() should never fail, because the vector has an inline capacity of 1.
    m_item_state_table_stack.append({});
}

ErrorOr<ParsedReportDescriptor> ReportDescriptorParser::parse()
{
    ParsedReportDescriptor parsed {};

    while (!m_stream.is_eof()) {
        auto item_header = TRY(m_stream.read_value<ItemHeader>());
        // dbgln("size: {:#b}, type: {:#b}, tag: {:#b}", item_header.size, to_underlying(item_header.type), item_header.tag);

        auto& current_item_state_table = m_item_state_table_stack.last();

        // XXX error messages

        switch (item_header.type) {
        case ItemType::Main:
            switch (static_cast<MainItemTag>(item_header.tag)) {
            case MainItemTag::Input: {
                auto input_item_data = TRY(read_item_data<InputItemData>(item_header));
                Vector<StringView> input_item_args;

                input_item_args.append(input_item_data.constant ? "Constant"sv : "Data"sv);
                input_item_args.append(input_item_data.variable ? "Variable"sv : "Array"sv);
                input_item_args.append(input_item_data.relative ? "Relative"sv : "Absolute"sv);
                input_item_args.append(input_item_data.wrap ? "Wrap"sv : "No Wrap"sv);
                input_item_args.append(input_item_data.non_linear ? "Non Linear"sv : "Linear"sv);
                input_item_args.append(input_item_data.no_preferred ? "No Preferred"sv : "Preferred State"sv);
                input_item_args.append(input_item_data.null_state ? "Null state"sv : "No Null position"sv);
                // Bit 7 is reserved
                input_item_args.append(input_item_data.buffered_bytes ? "Buffered Bytes"sv : "Bit Field"sv);

                dbgln("Input ({})", input_item_args);

                Field field {
                    .main_item_tag = MainItemTag::Input,
                    .item_state_table = current_item_state_table,
                };

                if (m_current_collection == nullptr)
                    TRY(parsed.children.try_empend(move(field)));
                else
                    TRY(m_current_collection->children.try_empend(move(field)));

                break;
            }
            case MainItemTag::Output: {
                auto output_item_data = TRY(read_item_data<OutputItemData>(item_header));
                Vector<StringView> input_item_args;

                input_item_args.append(output_item_data.constant ? "Constant"sv : "Data"sv);
                input_item_args.append(output_item_data.variable ? "Variable"sv : "Array"sv);
                input_item_args.append(output_item_data.relative ? "Relative"sv : "Absolute"sv);
                input_item_args.append(output_item_data.wrap ? "Wrap"sv : "No Wrap"sv);
                input_item_args.append(output_item_data.non_linear ? "Non Linear"sv : "Linear"sv);
                input_item_args.append(output_item_data.no_preferred ? "No Preferred"sv : "Preferred State"sv);
                input_item_args.append(output_item_data.null_state ? "Null state"sv : "No Null position"sv);
                input_item_args.append(output_item_data.volatile_ ? "Volatile"sv : "Non Volatile"sv);
                input_item_args.append(output_item_data.buffered_bytes ? "Buffered Bytes"sv : "Bit Field"sv);

                dbgln("Output ({})", input_item_args);
                break;
            }
            case MainItemTag::Feature:
                TODO();
                break;
            case MainItemTag::Collection: {
                auto raw_collection_type = TRY(read_item_data<LittleEndian<u8>>(item_header));
                dbgln("Collection ({:#x})", raw_collection_type);

                Collection new_collection {};

                new_collection.parent = m_current_collection;

                if (current_item_state_table.local.usages.size() > 1)
                    dbgln("Collection has more than one usage; don't know how to handle this");
                else if (!current_item_state_table.local.usages.is_empty())
                    new_collection.usage = current_item_state_table.local.usages.last();

                if (m_current_collection == nullptr) {
                    TRY(parsed.children.try_empend(move(new_collection)));
                    m_current_collection = parsed.children.last().get_pointer<Collection>();
                } else {
                    TRY(m_current_collection->children.try_empend(move(new_collection)));
                    m_current_collection = m_current_collection->children.last().get_pointer<Collection>();
                }

                break;
            }
            case MainItemTag::EndCollection:
                dbgln("End Collection");
                if (m_current_collection != nullptr)
                    m_current_collection = m_current_collection->parent;
                break;
            }

            // Reset the Local items in the current item state table.
            current_item_state_table.local = {};

            break;
        case ItemType::Global:
            switch (static_cast<GlobalItemTag>(item_header.tag)) {
            case GlobalItemTag::UsagePage:
                current_item_state_table.global.usage_page = TRY(read_item_data<LittleEndian<u16>>(item_header));
                dbgln("Usage Page ({:#x})", current_item_state_table.global.usage_page);
                break;
            case GlobalItemTag::LogicalMinimum:
                current_item_state_table.global.logical_minimum = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Logical Minimum ({:#x})", current_item_state_table.global.logical_minimum);
                break;
            case GlobalItemTag::LogicalMaximum:
                current_item_state_table.global.logical_maximum = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Logical Maximum ({:#x})", current_item_state_table.global.logical_maximum);
                break;
            case GlobalItemTag::PhysicalMinimum:
            case GlobalItemTag::PhysicalMaximum:
            case GlobalItemTag::UnitExponent:
            case GlobalItemTag::Unit:
                TODO();
                break;
            case GlobalItemTag::ReportSize:
                current_item_state_table.global.report_size = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Report Size ({:#x})", current_item_state_table.global.report_size);
                break;
                break;
            case GlobalItemTag::ReportID:
                current_item_state_table.global.report_id = TRY(read_item_data<LittleEndian<u8>>(item_header));
                dbgln("Report ID ({:#x})", current_item_state_table.global.report_id);
                break;
            case GlobalItemTag::ReportCount:
                current_item_state_table.global.report_count = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Report Count ({:#x})", current_item_state_table.global.report_count);
                break;
            case GlobalItemTag::Push:
            case GlobalItemTag::Pop:
                TODO();
                break;
            }
            break;
        case ItemType::Local:
            switch (static_cast<LocalItemTag>(item_header.tag)) {
            case LocalItemTag::Usage: {
                // XXX if (usage_id longer than 16 bits) set page as well
                auto usage = (static_cast<u32>(current_item_state_table.global.usage_page) << 16) | TRY(read_item_data<LittleEndian<u16>>(item_header));
                dbgln("Usage ({:#x})", usage);
                TRY(current_item_state_table.local.usages.try_append(usage));
                break;
            }
            case LocalItemTag::UsageMinimum:
                // XXX if (usage_id longer than 16 bits) set page as well
                current_item_state_table.local.usage_minimum = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Usage Minimum ({:#x})", current_item_state_table.local.usage_minimum);
                break;
            case LocalItemTag::UsageMaximum:
                // XXX if (usage_id longer than 16 bits) set page as well
                current_item_state_table.local.usage_maximum = TRY(read_item_data<LittleEndian<u32>>(item_header));
                dbgln("Usage Maximum ({:#x})", current_item_state_table.local.usage_maximum);
                break;
            case LocalItemTag::DesignatorIndex:
            case LocalItemTag::DesignatorMinimum:
            case LocalItemTag::DesignatorMaximum:
            case LocalItemTag::StringIndex:
            case LocalItemTag::StringMinimum:
            case LocalItemTag::StringMaximum:
            case LocalItemTag::Delimiter:
                TODO();
                break;
            }
            break;
        case ItemType::Reserved:
            if (item_header.tag == TAG_LONG_ITEM) {
                return Error::from_string_view_or_print_error_and_return_errno("long items are not supported"sv, EINVAL);
            } else {
                return Error::from_string_view_or_print_error_and_return_errno("unsupported reserved item"sv, EINVAL);
            }
        default:
            return Error::from_string_view_or_print_error_and_return_errno("unknown type"sv, EINVAL);
        }
    }

    return parsed;
}

template<typename T>
ErrorOr<T> ReportDescriptorParser::read_item_data(ItemHeader item_header)
{
    VERIFY(item_header.type != ItemType::Reserved && item_header.tag != TAG_LONG_ITEM);

    if (item_header.real_size() > sizeof(T))
        return Error::from_errno(EINVAL);

    // Short items have a max data length of 4.
    alignas(T) u8 buffer[4] {};
    TRY(m_stream.read_until_filled({ &buffer, item_header.real_size() }));

    return *bit_cast<T*>(&buffer);
}

}
