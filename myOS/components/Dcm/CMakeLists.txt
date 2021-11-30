#*************************************************************************
#    > File Name: Dcm.camkes
#    > SPDX-License-Identifier: BSD-2-Clause
#    > Author: XXX
#    > Created Time: 2021年11月17日 星期三 13时50分54秒
#************************************************************************/

cmake_minimum_required(VERSION 3.7.2)

project(ComponentDcm C)

DeclareCAmkESComponent(Dcm SOURCES src/Dcm.c INCLUDES include ../../include)
