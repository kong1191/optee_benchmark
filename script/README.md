OP-TEE benchmark performance analysis tool
---

performance_analysis.py can help to analysis the REE and TEE console outputs and generate a performance report.

Usage:

    $ python performance_analysis.py <logfile>


# Log file format

Script scan the logfile content and find out the data needed for performance analysis.
The following message patterns are used by the script to parse the logfile:

Printed by Host Application (simple_client):

* CounterTimerFrequency: `<counter frequency>`
* ClientOpenSession: start=`<counter value>`, enter_ta=`<counter value>`, return=`<counter value>`
* ClientInvokeCommand: start=`<counter value>`, enter_ta=`<counter value>`, return=`<counter value>`
* SecureSysCall: start=`<counter value>`, enter_svc=`<counter value>`, return=`<counter value>`

Printed by OP-TEE Linux driver:

* TEEDriver: do smc call, cmd=`<cmd number>`, cntvct=`<counter value>`
* TEEDriver: finish smc call, cmd=`<cmd number>`, cntvct=`<counter value>`

Printed by OP-TEE OS:

* TEECore: receiving smc call, cmd=`<cmd number>`, cntvct=`<counter value>`
* TEECore: finish smc call, cmd=`<cmd number>`, cntvct=`<counter value>`


# Performance Report

Here is a sample performance report.

The "Interval" field shows how long does it takes from previous step to current step.
For example, if we want to know how long doest it takes from "Client start Open Session command"
to "Driver start SMC call", we can look at the interval value at the same line as "Driver start
SMC Call" in the "Open Session" section, that is 398672(ns).
 
Interval = (CounterValueOfCurrentStep - CounterValueOfPreviousStep) x (CounterResolution)

```
$ cd script/
$ python performance_analysis.py sample_log.txt 
Counter resolution (ns) = 16

================================================================
Open Session
================================================================
When                           Counter Value    Interval(ns)
----------------------------------------------------------------
Client Start Command             19411438612               0
Driver Start SMC Call            19411463529          398672
TEE OS Receive SMC Call          19411497473          543104
TA Enter Command                 19412202128        11274480
TEE OS Finish SMC Call           19412296092         1503424
Driver Finish SMC Call           19412368547         1159280
Client Return From Command       19412416471          766784

----------------------------------------------------------------
Summary
----------------------------------------------------------------
Client -> TA (one way trip)    =   12216256 (ns)
Client -> Client (round trip)  =   15645744 (ns)
SMC Call Send To Secure OS     =     543104 (ns)
SMC Call Return From Secure OS =    1159280 (ns)


================================================================
Invoke Command
================================================================
When                           Counter Value    Interval(ns)
----------------------------------------------------------------
Client Start Command             19412494140               0
Driver Start SMC Call            19412517733          377488
TEE OS Receive SMC Call          19412551522          540624
TA Enter Command                 19412613966          999104
TEE OS Finish SMC Call           19412802652         3018976
Driver Finish SMC Call           19412859796          914304
Client Return From Command       19412920217          966736

----------------------------------------------------------------
Summary
----------------------------------------------------------------
Client -> TA (one way trip)    =    1917216 (ns)
Client -> Client (round trip)  =    6817232 (ns)
SMC Call Send To Secure OS     =     540624 (ns)
SMC Call Return From Secure OS =     914304 (ns)


================================================================
Secure System Call (TEE_GetPropertyAsBinaryBlock)
================================================================
When                             Counter Value    Interval(ns)
----------------------------------------------------------------
TA Invoke System Call              19413195393               0
Secure OS Receive System Call      19413225884          487856
TA Return From System Call         19413228214           37280

----------------------------------------------------------------
Summary
----------------------------------------------------------------
TA -> OS =     487856 (ns)
OS -> TA =      37280 (ns)
TA -> TA =     525136 (ns)

$
```