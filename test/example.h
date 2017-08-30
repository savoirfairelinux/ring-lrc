/*
 *  Copyright (C) 2017 Savoir-faire Linux Inc.
 *
 *  Author: Sébastien Blin <sebastien.blin@savoirfairelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 */

#pragma once

// cppunit
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// std
#include <memory>

// lrc
#include "api/lrc.h"

namespace ring
{
namespace test
{

class ExampleTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(ExampleTest);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();
public:
    void setUp();
    void test();

protected:
    std::unique_ptr<lrc::api::Lrc> lrc_;
};

} // namespace test
} // namespace ring
