
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_RELATIONSHIPS_HPP_
#define CXXREFLECT_METADATA_RELATIONSHIPS_HPP_

#include "cxxreflect/metadata/database.hpp"

namespace cxxreflect { namespace metadata {

    /// \defgroup cxxreflect_metadata_relationships Metadata -> Relationships
    ///
    /// Utility functions that map between tables in a metadata database
    ///
    /// These functions compute the targets of 1:1 and 1:N relationships in a metadata database.
    /// Some relationships are trivial to compute, for example, getting the range of fields defined
    /// by a given type definition.  These trivial relationships are defined directly in the row
    /// types:  in the case of the type -> field example, they are defined in the `type_def_row`
    /// class.
    ///
    /// The relationships mapped in this set of functions here are the nontrivial relationships.
    /// Each of these queries requires either a binary search over a metadata table to find a row
    /// or range of rows, or a thunk through a mapping table.
    ///
    /// We could define these as members of the row classes, but it is simpler to define them all
    /// here so they are defined in one place, and to distinguish them from the less expensive-to-
    /// call trivial relationships described before.
    ///
    /// For each 1:N relationship, there are three functions:
    ///
    ///  * `find_{relationship}_range`:  Computes the range of target values that map to the
    ///    parent (in effect, it computes an `equal_range`).
    ///
    ///  * `begin_{relationship}`:  Equivalent to `find_{relationship}_range(parent).first`.
    ///
    ///  * `end_{relationship}`:  Equivalent to `find_{relationship}_range(parent).second`.
    ///
    /// Because these sets of three functions always have the same meaning, we do not document them
    /// individually.
    ///
    /// @{






    auto find_owner_of_event(event_token const& element) -> type_def_row;
    auto find_owner_of_method_def(method_def_token const& element) -> type_def_row;
    auto find_owner_of_field(field_token const& element) -> type_def_row;
    auto find_owner_of_property(property_token const& element) -> type_def_row;
    auto find_owner_of_param(param_token const& element) -> method_def_row;





    /// Finds the constant value row that belongs to the given `parent`
    ///
    /// A `parent` may not have a constant value.  If the `parent` does not have a constant value,
    /// an uninitialized row is returned.  A `parent` may have at most one constant value.  If the
    /// `parent` has more than one associated constant value, a `metadata_error` is thrown.
    auto find_constant(has_constant_token const& parent) -> constant_row;

    /// Finds the field layout row that belongs to the given `parent` field
    ///
    /// A `parent` field may not have a layout row.  If the `parent` does not have a field layout
    /// row, an uninitialized row is returned.  A `parent` may have at most one field layout row.
    /// If the `parent` has more than one associated field layout row, a `metadata_error` is thrown.
    auto find_field_layout(field_token const& parent) -> field_layout_row;





    auto find_custom_attribute_range(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator_pair;
    auto begin_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator;
    auto end_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator;





    auto find_events_range(type_def_token const& parent) -> event_row_iterator_pair;
    auto begin_events(type_def_token const& parent) -> event_row_iterator;
    auto end_events(type_def_token const& parent) -> event_row_iterator;





    auto find_generic_params_range(type_or_method_def_token const& parent) -> generic_param_row_iterator_pair;
    auto find_generic_param(type_or_method_def_token const& parent, core::size_type index) -> generic_param_row;
    auto begin_generic_params(type_or_method_def_token const& parent) -> generic_param_row_iterator;
    auto end_generic_params(type_or_method_def_token const& parent) -> generic_param_row_iterator;





    auto find_generic_param_constraints_range(generic_param_token const& parent) -> generic_param_constraint_row_iterator_pair;
    auto begin_generic_param_constraints(generic_param_token const& parent) -> generic_param_constraint_row_iterator;
    auto end_generic_param_constraints(generic_param_token const& parent) -> generic_param_constraint_row_iterator;





    auto find_interface_impl_range(type_def_token const& parent) -> interface_impl_row_iterator_pair;
    auto begin_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator;
    auto end_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator;





    auto find_method_impl_range(type_def_token const& parent) -> method_impl_row_iterator_pair;
    auto begin_method_impls(type_def_token const& parent) -> method_impl_row_iterator;
    auto end_method_impls(type_def_token const& parent) -> method_impl_row_iterator;





    auto find_method_semantics_range(has_semantics_token const& parent) -> method_semantics_row_iterator_pair;
    auto begin_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator;
    auto end_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator;





    auto find_properties_range(type_def_token const& parent) -> property_row_iterator_pair;
    auto begin_properties(type_def_token const& parent) -> property_row_iterator;
    auto end_properties(type_def_token const& parent) -> property_row_iterator;





    /// @}

} }

#endif

// AMDG //
