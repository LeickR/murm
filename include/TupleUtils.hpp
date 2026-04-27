// MIT License
//
// Copyright (c) 2026 Leick Robinson (Virtual Science Workshop)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __MURM_TUPLE_UTILS_HPP__
#define __MURM_TUPLE_UTILS_HPP__


#include <cstdlib>
#include <iostream>
#include <tuple>

namespace murm {


// Create a template struct type whose size_t template arguments are <0, 1, 2, ...>
// (matching the template parameter pack passed in).
//
// For example:
//    typename TypesToIndices<T0, T1, T2>::GetIndices
//    evaluates to Indices<0, 1, 2>

// Indices template class
template <std::size_t... indices>
struct Indices {
    static void printMe() {
        int dummy[sizeof...(indices)] = { (std::cout << indices << " ", 0)... };
        std::cout << std::endl;
    }
};

// Helper template declaration
template <std::size_t next_index, typename IndicesT, typename... Types>
struct TypesToIndicesHelper;

// If there are no types left, terminate the recursion
template <std::size_t next_index, std::size_t... indices>
struct TypesToIndicesHelper<next_index, Indices<indices...>>
{
    typedef Indices<indices...> GetIndices;
};

// As long as there are types left, continue the recursion
template <std::size_t next_index, std::size_t... indices, typename Type, typename... Types>
struct TypesToIndicesHelper<next_index, Indices<indices...>, Type, Types...>
{
    typedef typename TypesToIndicesHelper<next_index+1, Indices<indices..., next_index>,
                                                      Types...>::GetIndices GetIndices;
};

// The template class where the type list is passed in
// (forward declaration)
template <typename... Types>
struct TypesToIndices {
    typedef typename TypesToIndicesHelper<0, Indices<>, Types...>::GetIndices GetIndices;
};



// If you're interested in the whys and wherefores of this implementation, then, for
// your perusal, here are a couple of first attempts at implementation that didn't work:

// This would be simpler, but it won't work because a primary class variadic template
// can have at most one parameter pack.

//// Forward declaration
//template <std::size_t... indices, typename... Types>
//struct Indices;
//
//// If all indices and no types, then it just sets "type" to itself
//template <std::size_t... indices>
//struct Indices<indices...>
//{
//   typedef Indices<indices...> converted;
//
//   static void printMe() {
//      ((std::cout << indices << " ";)...);
//      std::cout << std::endl;
//   }
//};
//
//// If at least one type and no indices, get this (recursive) party started
//template <typename Type, typename... Types>
//struct Indices<Type, Types...>
//{
//   typedef Indices<0, Types...>::converted converted;
//};
//
//// As long as there are types left, continue the recursion
//template <std::size_t... indices, std::size_t last_index, typename Type, typename... Types>
//struct Indices<indices..., last_index, Type, Types...>
//{
//   typedef Indices<indices..., last_index, last_index+1, Types...>::converted converted;
//};


// This also doesn't work, but I'm not sure why:
//
//// Indices template class
//template <std::size_t... indices>
//struct Indices {
//   static void printMe() {
//      int dummy[sizeof...(indices)] = { (std::cout << indices << " ", 0)... };
//      std::cout << std::endl;
//   }
//};
//
//// Helper template declaration
//template <typename IndicesT, typename... Types>
//struct TypesToIndicesHelper;
//
//// If there are no types left, terminate the recursion
//template <std::size_t... indices>
//struct TypesToIndicesHelper<Indices<indices...>>
//{
//   typedef Indices<indices...> GetIndices;
//};
//
//// As long as there are types left, continue the recursion
//template <std::size_t... indices, std::size_t last_index, typename Type, typename... Types>
//struct TypesToIndicesHelper<Indices<indices..., last_index>, Type, Types...>
//{
//   typedef typename TypesToIndicesHelper<Indices<indices..., last_index, last_index+1>, Types...>::GetIndices GetIndices;
//};
//
//// If at least one type and no indices yet, get this (recursive) party started
//template <typename Type, typename... Types>
//struct TypesToIndicesHelper<Indices<>, Type, Types...>
//{
//   typedef typename TypesToIndicesHelper<Indices<0>, Types...>::GetIndices GetIndices;
//};
//
//// The template class where the type list is passed in
//// (forward declaration)
//template <typename... Types>
//struct TypesToIndices {
//   typedef typename TypesToIndicesHelper<Indices<>, Types...>::GetIndices GetIndices;
//};


};

#endif // __MURM_TUPLE_UTILS_HPP__
