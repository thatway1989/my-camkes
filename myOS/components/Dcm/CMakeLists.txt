#*************************************************************************
#    > File Name: Dcm.camkes
#    > SPDX-License-Identifier: BSD-2-Clause
#    > Author: XXX
#    > Created Time: 2021年11月17日 星期三 13时50分54秒
#************************************************************************/

cmake_minimum_required(VERSION 3.7.2)

project(ComponentDcm C)

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(src DIR_SRCS)
DeclareCAmkESComponent(Dcm SOURCES src/Dcm.c ${DIR_SRCS} INCLUDES include ../../include)
