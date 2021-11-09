// SocketSpy.h
// author: Fabio Hellmann <fabio.hellmann@hcm-lab.de>
// created: 2021/10/05
// Copyright (C) University of Augsburg, Lab for Human Centered Artificial Intelligence
//
// *************************************************************************************************
//
// This file is part of Social Signal Interpretation (SSI) developed at the 
// Lab for Human Centered Multimedia of the University of Augsburg
//
// This library is free software; you can redistribute itand/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or any laterversion.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FORA PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along withthis library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//*************************************************************************************************

#include "gtest/gtest.h"
#include "ssi.h"

using namespace ssi;

TEST(TestCaseName, TestName) {
	EXPECT_EQ(1, 1);
	EXPECT_TRUE(true);
}

TEST(OptionListTest, AddOptionIllegalName) {
	OptionList list = OptionList();
	bool mybools[] = { false,true,false,true,false };
	bool result = list.addOption("option", &mybools, sizeof(mybools), SSI_BOOL, "this is my bool array");
	ASSERT_FALSE(result); // It should not be allowed to add an option with name "option"
}

TEST(OptionListTest, AddOptionIllegalDoubleEntry) {
	OptionList list = OptionList();
	bool mybools[] = { false,true,false,true,false };
	bool result = list.addOption("mybools", &mybools, sizeof(mybools), SSI_BOOL, "this is my bool array");
	ASSERT_TRUE(result); // Fine
	result = list.addOption("mybools", &mybools, sizeof(mybools), SSI_BOOL, "this is my bool array");
	ASSERT_FALSE(result); // It is not allowed to add an option with the same name
}

TEST(OptionListTest, AddOptionBool) {
	OptionList list = OptionList();
	bool mybools[] = { false,true,false,true,false };
	bool result = list.addOption("mybools", &mybools, sizeof(mybools), SSI_BOOL, "this is my bool array");
	ASSERT_TRUE(result); // Fine
	ssi_option_t* option = list.getOption("mybools");
	ASSERT_EQ(option->ptr, &mybools); // Pointers should be the same
}

/*
* bool mybools[] = {false,true,false,true,false};
	opts.addOption ("mybools", &mybools, 5, SSI_BOOL, "this is my bool array");

	int myints[] = {1,2,3,4,5};
	opts.addOption("myints", &myints, 5, SSI_INT, "this is my int array");

	unsigned int myuints[] = {1,2,3,4,5};
	opts.addOption("myuints", &myuints, 5, SSI_UINT, "this is my uint array");

	float myfloats[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
	opts.addOption("myfloats", myfloats, 5, SSI_FLOAT, "this is my float array");

	double mydoubles[] = {1.0, 2.0, 3.0, 4.0, 5.0};
	opts.addOption ("mydoubles", mydoubles, 5, SSI_DOUBLE, "this is my double array");

	char mychar = 'c';
	opts.addOption ("mychar", &mychar, 1, SSI_CHAR, "this is my char");

	char unsigned myuchar = 123;
	opts.addOption ("myuchar", &myuchar, 1, SSI_UCHAR, "this is my unsigned char");

	char mystring[256] = "mystring";
	opts.addOption("mystring", mystring, 256, SSI_CHAR, "this is my string", false);
*/