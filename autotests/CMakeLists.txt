# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

include(ECMAddTests)

ecm_add_tests(
    filenavigatortest.cpp
    viewerresolvertest.cpp
    LINK_LIBRARIES kpeekcore Qt6::Test
)
