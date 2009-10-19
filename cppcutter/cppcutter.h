/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CPPCUTTER_H__
#define __CPPCUTTER_H__

#include <cutter.h>
#include <cppcutter/cppcut-assertions.h>

/**
 * SECTION: cppcutter
 * @title: CppCutter
 * @short_description: Cutter with C++ support.
 * @see_also: <link
 *            linkend="cppcutter-cppcut-assertions">Assertions
 *            with C++ support</link>
 *
 * CppCutter provides C++ friendly interface of Cutter. If
 * you want to write tests with C++, it's good idea that you
 * consider CppCutter to be used too.
 *
 * It's easy to use CppCutter. You just include &lt;cppcutter.h&gt;
 * instead of &lt;cutter.h&gt; and use cppcutter.pc instead of
 * cutter.pc:
 *
 * test-xxx.cpp:
 * |[
 * -#include <cutter.h>
 * +#include <cppcutter.h>
 * ]|
 *
 * configure.ac:
 * |[
 * -AC_CHECK_CUTTER
 * +AC_CHECK_CPPCUTTER
 * ]|
 *
 * Makefile.am:
 * |[
 * -XXX_CFLAGS = $(CUTTER_CFLAGS)
 * -XXX_LIBS = $(CUTTER_LIBS)
 * +XXX_CFLAGS = $(CPPCUTTER_CFLAGS)
 * +XXX_LIBS = $(CPPCUTTER_LIBS)
 * ]|
 *
 * NOTE: You need to extern test functions in C scope. See
 * %CPPCUT_BEGIN_TEST_DECLS.
 */

/**
 * CPPCUTTER_ENABLED
 *
 * Defined when CppCutter is enabled.
 *
 * Since: 1.0.9
 */
#define CPPCUTTER_ENABLED 1

/**
 * CPPCUT_BEGIN_TEST_DECLS
 *
 * Use %CPPCUT_BEGIN_TEST_DECLS and %CPPCUT_END_TEST_DECLS
 * pair for prototype declarations for test functions:
 *
 * |[
 * CPPCUT_BEGIN_TEST_DECLS
 * void test_add ();
 * void test_remove ();
 * CPPCUT_END_TEST_DECLS
 * ]|
 *
 * Those macros just surround prototype declarations for
 * test functions with 'extern "C" {...}'.
 *
 * Since: 1.0.9
 */
#define CPPCUT_BEGIN_TEST_DECLS extern "C" {

/**
 * CPPCUT_END_TEST_DECLS
 *
 * See %CPPCUT_BEGIN_TEST_DECLS.
 *
 * Since: 1.0.9
 */
#define CPPCUT_END_TEST_DECLS }

#endif /* __CPPCUTTER_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/