import re
import sys


class OperationCounterInfo:
    def __init__(self, name):
        self.name = name
        self.client_start_cmd = 0L
        self.client_return_cmd = 0L
        self.driver_start_smc = 0L
        self.driver_finish_smc = 0L
        self.tee_os_receive_smc = 0L
        self.tee_os_finish_smc = 0L
        self.ta_enter_cmd = 0L

    def __repr__(self):
        tmp_str = self.name + "\n"
        tmp_str += "\tClient Start Command        =%d\n" % self.client_start_cmd
        tmp_str += "\tDriver Start SMC Call       =%d\n" % self.driver_start_smc
        tmp_str += "\tTEE OS Receive SMC Call     =%d\n" % self.tee_os_receive_smc
        tmp_str += "\tTA Enter Command            =%d\n" % self.ta_enter_cmd
        tmp_str += "\tTEE OS Finish SMC Call      =%d\n" % self.tee_os_finish_smc
        tmp_str += "\tDriver Finish SMC Call      =%d\n" % self.driver_finish_smc
        tmp_str += "\tClient Return From Command  =%d\n" % self.client_return_cmd
        return tmp_str

    def do_analysis(self, resolution_in_ns):
        tmp_str = "================================================================\n"
        tmp_str += self.name + "\n"
        tmp_str += "================================================================\n"
        tmp_str += "When                           Counter Value    Interval(ns)\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "Client Start Command        %16d%16d\n" % (self.client_start_cmd, 0)
        tmp_str += "Driver Start SMC Call       %16d%16d\n" % (self.driver_start_smc  , resolution_in_ns * (self.driver_start_smc   - self.client_start_cmd  ))
        tmp_str += "TEE OS Receive SMC Call     %16d%16d\n" % (self.tee_os_receive_smc, resolution_in_ns * (self.tee_os_receive_smc - self.driver_start_smc  ))
        tmp_str += "TA Enter Command            %16d%16d\n" % (self.ta_enter_cmd      , resolution_in_ns * (self.ta_enter_cmd       - self.tee_os_receive_smc))
        tmp_str += "TEE OS Finish SMC Call      %16d%16d\n" % (self.tee_os_finish_smc , resolution_in_ns * (self.tee_os_finish_smc  - self.ta_enter_cmd      ))
        tmp_str += "Driver Finish SMC Call      %16d%16d\n" % (self.driver_finish_smc , resolution_in_ns * (self.driver_finish_smc  - self.tee_os_finish_smc ))
        tmp_str += "Client Return From Command  %16d%16d\n" % (self.client_return_cmd , resolution_in_ns * (self.client_return_cmd  - self.driver_finish_smc ))
        tmp_str += "\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "Summary\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "Client -> TA (one way trip)    = %10d (ns)\n" % (resolution_in_ns * (self.ta_enter_cmd - self.client_start_cmd))
        tmp_str += "Client -> Client (round trip)  = %10d (ns)\n" % (resolution_in_ns * (self.client_return_cmd - self.client_start_cmd))
        tmp_str += "SMC Call Send To Secure OS     = %10d (ns)\n" % (resolution_in_ns * (self.tee_os_receive_smc - self.driver_start_smc))
        tmp_str += "SMC Call Return From Secure OS = %10d (ns)\n" % (resolution_in_ns * (self.driver_finish_smc - self.tee_os_finish_smc))
        tmp_str += "\n"
        return tmp_str


class SysCallCounterInfo:
    def __init__(self, name):
        self.name = name
        self.start = 0L
        self.enter = 0L
        self.finish = 0L

    def __repr__(self):
        tmp_str = self.name + "\n"
        tmp_str += "\tTA Invoke System Call      =%d\n" % self.start
        tmp_str += "\tTEE OS Receive System Call =%d\n" % self.enter
        tmp_str += "\tTA Return From System Call =%d\n" % self.finish
        return tmp_str

    def do_analysis(self, resolution_in_ns):
        tmp_str = "================================================================\n"
        tmp_str += self.name + "\n"
        tmp_str += "================================================================\n"
        tmp_str += "When                             Counter Value    Interval(ns)\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "TA Invoke System Call         %16d%16d\n" % (self.start, 0)
        tmp_str += "Secure OS Receive System Call %16d%16d\n" % (self.enter,  resolution_in_ns * (self.enter  - self.start))
        tmp_str += "TA Return From System Call    %16d%16d\n" % (self.finish, resolution_in_ns * (self.finish - self.enter))
        tmp_str += "\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "Summary\n"
        tmp_str += "----------------------------------------------------------------\n"
        tmp_str += "TA -> OS = %10d (ns)\n" % (resolution_in_ns * (self.enter - self.start))
        tmp_str += "OS -> TA = %10d (ns)\n" % (resolution_in_ns * (self.finish - self.enter))
        tmp_str += "TA -> TA = %10d (ns)\n" % (resolution_in_ns * (self.finish - self.start))
        tmp_str += "\n"
        return tmp_str


class CounterInfo:
    def __init__(self):
        self.freq = 0L
        self.open_session = OperationCounterInfo("Open Session")
        self.invoke_command = OperationCounterInfo("Invoke Command")
        self.secure_sys_call = SysCallCounterInfo("Secure System Call (TEE_GetPropertyAsBinaryBlock)")

    def __repr__(self):
        tmp_str = "Counter-Timer Frequency = %d (Hz)\n" % self.freq
        tmp_str += "\n"
        tmp_str += self.open_session.__repr__()
        tmp_str += "\n"
        tmp_str += self.invoke_command.__repr__()
        tmp_str += "\n"
        tmp_str += self.secure_sys_call.__repr__()
        return tmp_str

    def do_analysis(self):
        counter_resolution_in_ns = pow(10, 9) / counter_info.freq
        tmp_str = "Counter resolution (ns) = %d\n" % counter_resolution_in_ns
        tmp_str += "\n"
        tmp_str += self.open_session.do_analysis(counter_resolution_in_ns)
        tmp_str += "\n"
        tmp_str += self.invoke_command.do_analysis(counter_resolution_in_ns)
        tmp_str += "\n"
        tmp_str += self.secure_sys_call.do_analysis(counter_resolution_in_ns)
        tmp_str += "\n"
        return tmp_str

counter_info = CounterInfo()


def show_help():
    print "Usage: " + sys.argv[0] + " <filename>"


def is_counter_freq(line):
    result = re.search(r'CounterTimerFrequency:\s0x([0-9a-f]+)', line)
    if result:
        counter_info.freq = long(result.group(1), 16)
        return True
    else:
        return False


def is_client_open_session(line):
    result = re.search(r'ClientOpenSession:\sstart=0x([0-9a-f]+),\senter_ta=0x([0-9a-f]+),\sreturn=0x([0-9a-f]+)', line)
    if result:
        counter_info.open_session.client_start_cmd = long(result.group(1), 16)
        counter_info.open_session.ta_enter_cmd = long(result.group(2), 16)
        counter_info.open_session.client_return_cmd = long(result.group(3), 16)
        return True
    else:
        return False


def is_client_invoke_command(line):
    result = re.search(r'ClientInvokeCommand:\sstart=0x([0-9a-f]+),\senter_ta=0x([0-9a-f]+),\sreturn=0x([0-9a-f]+)',
                       line)
    if result:
        counter_info.invoke_command.client_start_cmd = long(result.group(1), 16)
        counter_info.invoke_command.ta_enter_cmd = long(result.group(2), 16)
        counter_info.invoke_command.client_return_cmd = long(result.group(3), 16)
        return True
    else:
        return False


def is_secure_sys_call(line):
    result = re.search(r'SecureSysCall:\sstart=0x([0-9a-f]+),\senter_svc=0x([0-9a-f]+),\sreturn=0x([0-9a-f]+)', line)
    if result:
        counter_info.secure_sys_call.start = long(result.group(1), 16)
        counter_info.secure_sys_call.enter = long(result.group(2), 16)
        counter_info.secure_sys_call.finish = long(result.group(3), 16)
        return True
    else:
        return False


def is_tee_driver_do_smc_call(line):
    result = re.search(r'TEEDriver:\sdo\ssmc\scall,\scmd=([0-9]),\scntvct=0x([0-9a-f]+)', line)
    if result:
        cmd = int(result.group(1))
        if cmd == 0:  # Open Session
            if counter_info.open_session.driver_start_smc == 0L:  # uninitialized
                counter_info.open_session.driver_start_smc = long(result.group(2), 16)
        elif cmd == 1:  # Invoke Command
            if counter_info.invoke_command.driver_start_smc == 0L:  # uninitialized
                counter_info.invoke_command.driver_start_smc = long(result.group(2), 16)
        elif cmd == 2:  # Close Session
            pass
        else:
            print "Unknown SMC call command = %d" % cmd

        return True
    else:
        return False


def is_tee_driver_finish_smc_call(line):
    result = re.search(r'TEEDriver:\sfinish\ssmc\scall,\scmd=([0-9]),\scntvct=0x([0-9a-f]+)', line)
    if result:
        cmd = int(result.group(1))
        if cmd == 0:  # Open Session
            if counter_info.open_session.driver_finish_smc == 0L:  # uninitialized
                counter_info.open_session.driver_finish_smc = long(result.group(2), 16)
        elif cmd == 1:  # Invoke Command
            if counter_info.invoke_command.driver_finish_smc == 0L:  # uninitialized
                counter_info.invoke_command.driver_finish_smc = long(result.group(2), 16)
        elif cmd == 2:  # Close Session
            pass
        else:
            print "Unknown SMC call command = %d" % cmd

        return True
    else:
        return False


def is_tee_core_receive_smc_call(line):
    result = re.search(r'TEECore:\sreceiving\ssmc\scall,\scmd=([0-9]),\scntvct=0x([0-9a-f]+)', line)
    if result:
        cmd = int(result.group(1))
        if cmd == 0:  # Open Session
            if counter_info.open_session.tee_os_receive_smc == 0L:  # uninitialized
                counter_info.open_session.tee_os_receive_smc = long(result.group(2), 16)
        elif cmd == 1:  # Invoke Command
            if counter_info.invoke_command.tee_os_receive_smc == 0L:  # uninitialized
                counter_info.invoke_command.tee_os_receive_smc = long(result.group(2), 16)
        elif cmd == 2:  # Close Session
            pass
        else:
            print "Unknown SMC call command = %d" % cmd

        return True
    else:
        return False


def is_tee_core_finish_smc_call(line):
    result = re.search(r'TEECore:\sfinish\ssmc\scall,\scmd=([0-9]),\scntvct=0x([0-9a-f]+)', line)
    if result:
        cmd = int(result.group(1))
        if cmd == 0:  # Open Session
            if counter_info.open_session.tee_os_finish_smc == 0L:  # uninitialized
                counter_info.open_session.tee_os_finish_smc = long(result.group(2), 16)
        elif cmd == 1:  # Invoke Command
            if counter_info.invoke_command.tee_os_finish_smc == 0L:  # uninitialized
                counter_info.invoke_command.tee_os_finish_smc = long(result.group(2), 16)
        elif cmd == 2:  # Close Session
            pass
        else:
            print "Unknown SMC call command = %d" % cmd

        return True
    else:
        return False


def get_counter_data(line):
    if is_counter_freq(line):
        pass
    elif is_client_open_session(line):
        pass
    elif is_client_invoke_command(line):
        pass
    elif is_secure_sys_call(line):
        pass
    elif is_tee_driver_do_smc_call(line):
        pass
    elif is_tee_driver_finish_smc_call(line):
        pass
    elif is_tee_core_receive_smc_call(line):
        pass
    elif is_tee_core_finish_smc_call(line):
        pass
    else:
        pass


def do_parse(f_object):
    try:
        while True:
            line = f_object.next()
            get_counter_data(line)
    except StopIteration:
        pass


# Main #
if len(sys.argv) < 2:
    show_help()
    exit()

try:
    filename = sys.argv[1]
    f = open(filename, 'r')
    do_parse(f)
    f.close()
except IOError:
    print "can not open file"

#print counter_info
print counter_info.do_analysis()

