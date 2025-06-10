#! /usr/bin/env python3

from dataclasses import asdict, dataclass
from threading import Thread, Lock
import enum
import time
import subprocess

import requests


MCU_URL = "http://10.253.0.132"


JETSON_VENDOR = "0955"
JETSON_RCM_PRODUCT = "7c18"
JETSON_NORMAL_PRODUCT = "7020"

LINE_UP = "\033[1A"
LINE_CLEAR = "\x1b[2K"


class bcolors:
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def colored(color: str, text: str) -> str:
    return f"{color}{text}{bcolors.ENDC}"


def colored_bool(b: bool) -> str:
    return colored(bcolors.OKGREEN if b else bcolors.FAIL, str(b))


def clear_line():
    print(LINE_UP, end=LINE_CLEAR)


class JetsonState(enum.Enum):
    RECOVERY = "RECOVERY"
    NORMAL = "NORMAL"
    NA = "NA"


@dataclass
class McuState:
    power: bool
    recovery: bool


def make_mcu_request(endpoint: str) -> McuState:
    resp = requests.get(f"{MCU_URL}{endpoint}")

    if resp.status_code != requests.codes.ok:
        raise RuntimeError(f"Request {endpoint} failed: {resp.status_code}")

    return McuState(**resp.json())


def toggle_power(state: McuState) -> McuState:
    ep = "/power/on" if not state.power else "/power/off"
    return make_mcu_request(ep)


def toggle_recovery(state: McuState) -> McuState:
    ep = "/recovery/on" if not state.recovery else "/recovery/off"
    return make_mcu_request(ep)


def press_pwr(_=None) -> McuState:
    return make_mcu_request("/press_power_btn")


def get_state(_=None) -> McuState:
    return make_mcu_request("/state")


ACTIONS = {
    "TOGGLE_POWER": toggle_power,
    "TOGGLE_RECOVERY": toggle_recovery,
    "PRESS_PWR_BTN": press_pwr,
    "REFRESH_STATE": get_state,
}


def prompt(max_n: int) -> int:
    while True:
        inp = input()
        try:
            v = int(inp.strip())
        except ValueError:
            v = None

        if v is None or v > max_n:
            print("Invalid input, try again...")
            time.sleep(0.5)
            clear_line()
            clear_line()
            continue

        # actions displayed as 1 indexed, reduce
        return v - 1


def look_for_jetson():
    id_name_pairs = [
        ln[ln.index("ID") + 3 :].strip().split(" ", 1)
        for ln in subprocess.check_output("lsusb", text=True).splitlines()
    ]

    for vidpid, _ in id_name_pairs:
        vid, pid = vidpid.split(":")
        if vid == JETSON_VENDOR:
            if pid == JETSON_RCM_PRODUCT:
                return JetsonState.RECOVERY
            elif pid == JETSON_NORMAL_PRODUCT:
                return JetsonState.NORMAL

    return JetsonState.NA


def main_loop():
    mcu_state = get_state()
    jetson_state = JetsonState.NA
    term_lock = Lock()

    action_names = list(ACTIONS.keys())
    action_funcs = list(ACTIONS.values())

    # 2 for prompt / action print + 2 for header + jetson line
    nlines_to_reset = len(action_funcs) + 2 + 2
    nlines_without_action = nlines_to_reset - 2
    curr_n_to_reset = nlines_without_action

    def clear_term():
        for _ in range(curr_n_to_reset):
            clear_line()

    def print_interface():
        nonlocal curr_n_to_reset
        s = " ".join(f"{k}: {colored_bool(v)}" for k, v in asdict(mcu_state).items())
        jstr = jetson_state.name
        match jetson_state:
            case JetsonState.NA:
                jstr = colored(bcolors.FAIL, jstr)
            case JetsonState.NORMAL:
                jstr = colored(bcolors.OKBLUE, jstr)
            case JetsonState.RECOVERY:
                jstr = colored(bcolors.WARNING, jstr)

        print(s)
        print("Jetson:", jstr)

        for i, k in enumerate(action_names):
            print(f"({i+1})", k)

        curr_n_to_reset = nlines_without_action

    def jetson_loop():
        nonlocal jetson_state
        while True:
            jetson_state = look_for_jetson()
            with term_lock:
                clear_term()
                print_interface()
            time.sleep(3)

    Thread(target=jetson_loop, daemon=True).start()

    while True:
        with term_lock:
            print_interface()

        i = prompt(max_n=len(action_funcs))

        with term_lock:
            curr_n_to_reset = nlines_to_reset
            print("Executing", colored(bcolors.OKBLUE, action_names[i]))
            mcu_state = action_funcs[i](mcu_state)

        time.sleep(0.4)
        with term_lock:
            clear_term()


def main():
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\nExiting")


if __name__ == "__main__":
    main()
