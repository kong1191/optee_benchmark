Simple TA for OP-TEE benchmark
---

This simple Trusted Application serves the purpose of showing how you could open up functionality from a TEE or just simply how to interact with a TEE using GlobalPlatform interfaces. In this case we have been using OP-TEE as the TEE, but since OP-TEE is a GlobalPlatform compliant TEE. It should be possible to run the same application on other GlobalPlatform compliant TEEs also.

Building and running the TA
---
#### Building
Easiest way to test this is to run the script [setup_qemu_optee.sh](https://github.com/OP-TEE/optee_os/blob/master/scripts/setup_qemu_optee.sh) that could be found in the [optee_os](https://github.com/OP-TEE/optee_os) git at GitHub. This script will setup the complete environment needed to run OP-TEE on QEMU. When setup script has been done, you will need to clone this git into the same root folder as you pointed out in the setup script.

In this git (optee_benchmark.git), there is the script [build_app.sh](build_app.sh) which we recommend that you use when build, since that sets the needed flags and points to the correct folders. You can of course call make directly on the commandline, but you will have to point out all the other things mentioned in "build_app.sh" in any case. When this has been setup, you should be able to build the Trusted Application.

Next step is to get the newly built Trusted Application into the filesystem that is used when booting QEMU. To do this you need to edit the file ```gen_rootfs/filelist-tee.txt``adding lines telling where to find and where to store the test binary and the Trusted Application, like this:

```
# TA's
....
file /lib/teetz/99e937a0-8f3e-11e4-8b8f0002a5d5c51b.ta /home/johndoe/devel/qemu_optee/optee_benchmark/ta/out-client-aarch32/99e937a0-8f3e-11e4-8b8f0002a5d5c51b.ta 444 0 0

# OP-TEE Tests
...
file /bin/simple_client /home/johndoe/devel/qemu_optee/optee_benchmark/host/simple_client 755 0 0
```

When that has been done and the files has been saved, you need to regenerate the filesystem. That could be done by running the script ```build.sh```. For you who wonder about the strange name of the Trusted Application, we can mention that this is coming from the UUID for the particular Trusted Application.

#### Running
1. Run ```serial_0.sh``` and ```serial_1.sh``` in different console.

2. Open another console and start booting up QEMU by running ```run_qemu.sh``` and wait until you have a shell prompt.

3. Load the kernel module for OP-TEE and launch tee-supplicant, by typing:

   ```
   $ modprobe optee
   $ tee-supplicant &
   ```
4. Launch the test (and thereby the TA), by typing:

   ```
   $ simple_client
   ```

