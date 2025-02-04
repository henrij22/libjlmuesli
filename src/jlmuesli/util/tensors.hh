// SPDX-FileCopyrightText: 2025 Henrik Jakob jakob@ibb.uni-stuttgart.de
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <jlmuesli/util/common.hh>
#include <muesli/muesli.h>
#include <jlcxx/jlcxx.hpp>

// Function declarations
void registerIVector(jlcxx::Module& mod);
void registerITensor(jlcxx::Module& mod);
void registerTensors(jlcxx::Module& mod);