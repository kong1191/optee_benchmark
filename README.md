Simple client and simple TA for OP-TEE benchmark
---

This simple host/trusted application serves the purpose of measuring the performance of OP-TEE.

Building and running the applications on QEMU
---
#### Building
Easiest way to test this is to run the script [setup_qemu_optee.sh](script/setup_qemu_optee.sh). This script will setup the complete environment needed to run OP-TEE on QEMU.

When that has been done and the files has been saved, you need to regenerate the filesystem. That could be done by running the script ```build.sh```.

#### Running
1. Run ```serial_0.sh``` and ```serial_1.sh``` in different console.

2. Open another console and start booting up QEMU by running ```run_qemu.sh``` and wait until you have a shell prompt.

3. Load the kernel module for OP-TEE and launch tee-supplicant, by typing:

   ```
   $ modprobe optee_armtz
   $ tee-supplicant &
   ```
4. Launch the test (and thereby the TA), by typing:

   ```
   $ simple_client
   ```

Building and running the applications on FVP
---
#### Building
Easiest way to test this is to run the script [setup_fvp_optee.sh](script/setup_fvp_optee.sh). This script will setup the complete environment needed to run OP-TEE on FVP.

When that has been done and the files has been saved, you need to regenerate the filesystem. That could be done by running the script ```build_secure.sh```, ```optee_benchmark/build_app.sh``` and then ```build_normal.sh```. 

#### Running

1. Open console and start booting up FVP by running ```run_foundation.sh``` and wait until you have a shell prompt.

2. Load the kernel module for OP-TEE and launch tee-supplicant, by typing:

   ```
   $ modprobe optee_armtz
   $ tee-supplicant &
   ```
3. Launch the test (and thereby the TA), by typing:

   ```
   $ simple_client
   ```

