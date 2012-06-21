
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/relationships.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata {

    auto find_owner_of_event(event_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        auto const map_row(detail::get_owning_row<table_id::event_map, table_id::event>(
            element,
            column_id::event_map_first_event));

        return row_from(map_row.parent());
    }
    
    auto find_owner_of_method_def(method_def_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::type_def, table_id::method_def>(
            element,
            column_id::type_def_first_method);
    }

    auto find_owner_of_field(field_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::type_def, table_id::field>(
            element,
            column_id::type_def_first_field);
    }

    auto find_owner_of_property(property_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        auto const map_row(detail::get_owning_row<table_id::property_map, table_id::property>(
            element,
            column_id::property_map_first_property));

        return row_from(map_row.parent());
    }

    auto find_owner_of_param(param_token const& element) -> method_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::method_def, table_id::param>(
            element,
            column_id::method_def_first_parameter);
    }





    auto find_constant(has_constant_token const& parent) -> constant_row
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_constant,
            table_id::constant,
            column_id::constant_parent));

        // Not every row has a constant value:
        if (range.empty())
            return constant_row();

        // If a row has a constant, it must have exactly one:
        database_table const& constant_table(parent.scope().tables()[table_id::constant]);
        if (range.size() != constant_table.row_size())
            throw core::metadata_error(L"constant table has non-unique parent index");

        return create_row<constant_row>(&parent.scope(), begin(range));
    }

    auto find_field_layout(field_token const& parent) -> field_layout_row
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::field,
            table_id::field_layout,
            column_id::field_layout_parent));

        // Not every row has a field layout value:
        if (range.empty())
            return field_layout_row();

        // If a row has a field layout, it must have exactly one:
        database_table const& field_layout_table(parent.scope().tables()[table_id::field_layout]);
        if (range.size() != field_layout_table.row_size())
            throw core::metadata_error(L"field layout table has non-unique parent index");

        return create_row<field_layout_row>(&parent.scope(), begin(range));
    }





    auto find_custom_attribute_range(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_custom_attribute,
            table_id::custom_attribute,
            column_id::custom_attribute_parent));

        return std::make_pair(
            custom_attribute_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            custom_attribute_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }
    
    auto begin_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator
    {
        return find_custom_attribute_range(parent).first;
    }

    auto end_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator
    {
        return find_custom_attribute_range(parent).second;
    }





    auto find_events_range(type_def_token const& parent) -> event_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::event_map,
            column_id::event_map_parent));

        // Not every type has events; if this is such a type, return an empty range:
        if (range.empty())
            return std::make_pair(
                event_row_iterator(&parent.scope(), 0),
                event_row_iterator(&parent.scope(), 0));

        // If a row has an event_map row, it must have exactly one:
        database_table const& event_map_table(parent.scope().tables()[table_id::event_map]);
        if (range.size() != event_map_table.row_size())
            throw core::metadata_error(L"event map table has non-unique parent index");

        return std::make_pair(
            event_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            event_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }

    auto begin_events(type_def_token const& parent) -> event_row_iterator
    {
        return find_events_range(parent).first;
    }

    auto end_events(type_def_token const& parent) -> event_row_iterator
    {
        return find_events_range(parent).second;
    }





    auto find_interface_impl_range(type_def_token const& parent) -> interface_impl_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::interface_impl,
            column_id::interface_impl_parent));

        return std::make_pair(
            interface_impl_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            interface_impl_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }

    auto begin_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator
    {
        return find_interface_impl_range(parent).first;
    }

    auto end_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator
    {
        return find_interface_impl_range(parent).second;
    }





    auto find_method_impl_range(type_def_token const& parent) -> method_impl_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::method_impl,
            column_id::method_impl_parent));
        
        return std::make_pair(
            method_impl_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            method_impl_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }
    
    auto begin_method_impls(type_def_token const& parent) -> method_impl_row_iterator
    {
        return find_method_impl_range(parent).first;
    }

    auto end_method_impls(type_def_token const& parent) -> method_impl_row_iterator
    {
        return find_method_impl_range(parent).second;
    }





    auto find_method_semantics_range(has_semantics_token const& parent) -> method_semantics_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_semantics,
            table_id::method_semantics,
            column_id::method_semantics_parent));

        return std::make_pair(
            method_semantics_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            method_semantics_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }

    auto begin_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator
    {
        return find_method_semantics_range(parent).first;
    }

    auto end_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator
    {
        return find_method_semantics_range(parent).second;
    }





    auto find_properties_range(type_def_token const& parent) -> property_row_iterator_pair
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::property_map,
            column_id::property_map_parent));

        // Not every type has events; if this is such a type, return an empty range:
        if (range.empty())
            return std::make_pair(
                property_row_iterator(&parent.scope(), 0),
                property_row_iterator(&parent.scope(), 0));

        // If a row has an property_map row, it must have exactly one:
        database_table const& property_map_table(parent.scope().tables()[table_id::property_map]);
        if (range.size() != property_map_table.row_size())
            throw core::metadata_error(L"property map table has non-unique parent index");

        return std::make_pair(
            property_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            property_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }

    auto begin_properties(type_def_token const& parent) -> property_row_iterator
    {
        return find_properties_range(parent).first;
    }

    auto end_properties(type_def_token const& parent) -> property_row_iterator
    {
        return find_properties_range(parent).second;
    }

} }

// AMDG //