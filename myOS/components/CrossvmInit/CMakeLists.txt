#*************************************************************************
#    > File Name: Dcm.camkes
#    > SPDX-License-Identifier: BSD-2-Clause
#    > Author: XXX
#    > Created Time: 2021年11月17日 星期三 13时50分54秒
#************************************************************************/

cmake_minimum_required(VERSION 3.7.2)

project(CrossvmInit C)

# Define our demo component that shares a dataport with the VM
DeclareCAmkESComponent(CrossvmInit SOURCES src/crossvm_init.c INCLUDES ../../include)
