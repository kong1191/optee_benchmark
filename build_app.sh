#!/bin/bash
export PATH=$HOME/devel/qemu_optee/toolchains/aarch32/bin:$PATH

export TA_DEV_KIT_DIR=$HOME/devel/qemu_optee/out-os-qemu/export-user_ta
export TEEC_EXPORT=$HOME/devel/qemu_optee/out-client-armv7/export

cd $HOME/devel/qemu_optee/optee_benchmark
make O=./out-client-aarch32 \
                HOST_CROSS_COMPILE=arm-linux-gnueabihf- \
                TA_CROSS_COMPILE=arm-linux-gnueabihf- \
                $@
