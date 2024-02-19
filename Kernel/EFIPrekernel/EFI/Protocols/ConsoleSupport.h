/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/EFIPrekernel/EFI/EFI.h>

// https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html

namespace EFI {

// EFI_INPUT_KEY: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#efi-simple-text-input-protocol-readkeystroke
struct InputKey {
    u16 scan_code;
    char16_t unicode_char;
};
static_assert(AssertSize<InputKey, 4>());

// EFI_SIMPLE_TEXT_INPUT_PROTOCOL: https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#simple-text-input-protocol
struct SimpleTextInputProtocol {
    static constexpr GUID guid = { 0x387477c1, 0x69c7, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

    using InputResetFn = EFIAPI Status (*)(SimpleTextInputProtocol*, Boolean extended_verification);
    using InputReadKeyFn = EFIAPI Status (*)(SimpleTextInputProtocol*, InputKey* key);

    InputResetFn reset;
    InputReadKeyFn read_key_stroke;
    Event wait_for_key;
};
static_assert(AssertSize<SimpleTextInputProtocol, 24>());

// https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#simple-text-output-protocol

// SIMPLE_TEXT_OUTPUT_MODE: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#simple-text-output-protocol
struct SimpleTextOutputMode {
    i32 max_mode;
    i32 mode;
    i32 attribute;
    i32 cursor_column;
    i32 cursor_row;
    Boolean cursor_visible;
};
static_assert(AssertSize<SimpleTextOutputMode, 24>());

// EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL: https://uefi.org/specs/UEFI/2.10/12_Protocols_Console_Support.html#simple-text-output-protocol
struct SimpleTextOutputProtocol {
    static constexpr GUID guid = { 0x387477c2, 0x69c7, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

    using TextResetFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, Boolean extended_verification);
    using TextStringFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, char16_t* string);
    using TextTestStringFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, char16_t* string);
    using TextQueryModeFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, FlatPtr mode_number, FlatPtr* columns, FlatPtr* rows);
    using TextSetModeFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, FlatPtr mode_number);
    using TextSetAttributeFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, FlatPtr attribute);
    using TextClearScreenFn = EFIAPI Status (*)(SimpleTextOutputProtocol*);
    using TextSetCursorPositionFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, FlatPtr column, FlatPtr row);
    using TextEnableCursorFn = EFIAPI Status (*)(SimpleTextOutputProtocol*, Boolean visible);

    TextResetFn reset;
    TextStringFn output_string;
    TextTestStringFn test_string;
    TextQueryModeFn query_mode;
    TextSetModeFn set_mode;
    TextSetAttributeFn set_attribute;
    TextClearScreenFn clear_screeen;
    TextSetCursorPositionFn set_cursor_position;
    TextEnableCursorFn enable_cursor;
    SimpleTextOutputMode* mode;
};
static_assert(AssertSize<SimpleTextOutputProtocol, 80>());
static_assert(__builtin_offsetof(SimpleTextOutputProtocol, output_string) == 8);

}
