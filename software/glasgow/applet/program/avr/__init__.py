"""
The Microchip (Atmel) AVR family has 9 (nine) incompatible programming interfaces. The vendor
provides no overview, compatibility matrix, or (for most interfaces) documentation other than
descriptions in MCU datasheets, so this document has to fill in the blanks.

The table below contains the summary of all necessary information to understand and implement these
programming interfaces (with the exception of debugWIRE). The wire counts include all wires between
the programmer and the target, including ~RESET, but excluding power, ground, and xtal (if any).

  * "Low-voltage serial"; 4-wire.
    Described in AVR910 application note and e.g. ATmega8 datasheet.
    This is what is commonly called SPI programming interface.
  * "Parallel"; 16-wire; requires +12 V on ~RESET.
    Described in e.g. ATmega8 datasheet.
  * JTAG; 4-wire.
    Described in e.g. ATmega323 datasheet.
  * "High-voltage serial"; 5-wire; requires 12 V on ~RESET.
    Described in e.g. ATtiny11 datasheet.
  * debugWIRE; 1-wire.
    Completely undocumented, partially reverse-engineered.
  * TPI ("tiny programming interface"); 3-wire.
    Described in AVR918 application note and e.g. ATtiny4 datasheet.
  * PDI ("program/debug interface"); 2-wire.
    Described in AVR1612 application note and e.g. ATxmega32D4 datasheet.
    PDI command set is a non-strict superset of TPI command set. PDICLK is unified with ~RESET.
  * UPDI ("unified program/debug interface"); 1-wire.
    Described in e.g. ATtiny417 datasheet.
    UPDI command set is a non-strict subset of PDI command set. PDICLK and PDIDATA are unified
    with ~RESET.
  * aWire; 1-wire; AVR32 only.
    Described in e.g. AT32UC3L064 datasheet.
"""

import sys
import logging
import argparse
from abc import ABCMeta, abstractmethod
from fx2.format import autodetect, input_data, output_data

from ... import *
from ....database.microchip.avr import *


__all__ = ["ProgramAVRError", "ProgramAVRInterface", "ProgramAVRApplet"]


class ProgramAVRError(GlasgowAppletError):
    pass


class ProgramAVRInterface(metaclass=ABCMeta):
    @abstractmethod
    async def programming_enable(self):
        raise NotImplementedError

    @abstractmethod
    async def programming_disable(self):
        raise NotImplementedError

    @abstractmethod
    async def read_signature(self):
        raise NotImplementedError

    @abstractmethod
    async def read_fuse(self, address):
        raise NotImplementedError

    async def read_fuse_range(self, addresses):
        return bytearray([await self.read_fuse(address) for address in addresses])

    @abstractmethod
    async def write_fuse(self, address, data):
        raise NotImplementedError

    @abstractmethod
    async def read_lock_bits(self):
        raise NotImplementedError

    @abstractmethod
    async def write_lock_bits(self, data):
        raise NotImplementedError

    @abstractmethod
    async def read_calibration(self, address):
        raise NotImplementedError

    async def read_calibration_range(self, addresses):
        return bytearray([await self.read_calibration(address) for address in addresses])

    @abstractmethod
    async def read_program_memory(self, address):
        raise NotImplementedError

    async def read_program_memory_range(self, addresses):
        return bytearray([await self.read_program_memory(address) for address in addresses])

    @abstractmethod
    async def load_program_memory_page(self, address, data):
        raise NotImplementedError

    @abstractmethod
    async def write_program_memory_page(self, address):
        raise NotImplementedError

    async def write_program_memory_range(self, address, chunk, page_size):
        dirty_page = False
        page_mask  = page_size - 1

        for offset, byte in enumerate(chunk):
            byte_address = address + offset
            if dirty_page and byte_address % page_size == 0:
                await self.write_program_memory_page((byte_address - 1) & ~page_mask)

            await self.load_program_memory_page(byte_address & page_mask, byte)
            dirty_page = True

        if dirty_page:
            await self.write_program_memory_page(byte_address & ~page_mask)

    @abstractmethod
    async def read_eeprom(self, address):
        raise NotImplementedError

    async def read_eeprom_range(self, addresses):
        return bytearray([await self.read_eeprom(address) for address in addresses])

    @abstractmethod
    async def load_eeprom_page(self, address, data):
        raise NotImplementedError

    @abstractmethod
    async def write_eeprom_page(self, address):
        raise NotImplementedError

    async def write_eeprom_range(self, address, chunk, page_size):
        dirty_page = False
        page_mask  = page_size - 1

        for offset, byte in enumerate(chunk):
            byte_address = address + offset
            if dirty_page and byte_address % page_size == 0:
                await self.write_eeprom_page((byte_address - 1) & ~page_mask)

            await self.load_eeprom_page(byte_address & page_mask, byte)
            dirty_page = True

        if dirty_page:
            await self.write_eeprom_page(byte_address & ~page_mask)

    @abstractmethod
    async def chip_erase(self):
        raise NotImplementedError


class ProgramAVRApplet(GlasgowApplet):
    logger = logging.getLogger(__name__)
    help = "program Microchip (Atmel) AVR microcontrollers"
    description = """
    Commands that read or write memory contents derive the file format from the filename as follows:

    ::
        *.bin                   binary; as-is
        *.hex *.ihx *.ihex      Intel HEX
        - (stdout)              hex dump when writing to a terminal, binary otherwise
    """

    @classmethod
    def add_interact_arguments(cls, parser):
        extensions = ", ".join([".bin", ".hex", ".ihx", ".ihex"])

        def bits(arg): return int(arg, 2)

        def memory_file(kind, argparse_file):
            def argument(arg):
                file = argparse_file(arg)
                if file.fileno() == sys.stdout.fileno():
                    fmt = "hex" if file.isatty() else "bin"
                else:
                    try:
                        fmt = autodetect(file)
                    except ValueError:
                        raise argparse.ArgumentTypeError(
                            f"cannot determine format of {kind} file {file.name!r} from extension; "
                            f"recognized extensions are: {extensions}")
                return file, fmt
            return argument

        extension_help = f"(must be '-' or end with: {extensions})"

        p_operation = parser.add_subparsers(dest="operation", metavar="OPERATION")

        p_identify = p_operation.add_parser(
            "identify", help="identify connected device")

        p_read = p_operation.add_parser(
            "read", help="read device memories")
        p_read.add_argument(
            "-f", "--fuses", default=False, action="store_true",
            help="display fuse bytes")
        p_read.add_argument(
            "-l", "--lock-bits", default=False, action="store_true",
            help="display lock bits")
        p_read.add_argument(
            "-c", "--calibration", default=False, action="store_true",
            help="display calibration bytes")
        p_read.add_argument(
            "-p", "--program", metavar="FILE",
            type=memory_file("program memory", argparse.FileType("wb")),
            help=f"write program memory contents to FILE {extension_help}")
        p_read.add_argument(
            "-e", "--eeprom", metavar="FILE",
            type=memory_file("EEPROM", argparse.FileType("wb")),
            help=f"write EEPROM contents to FILE {extension_help}")

        p_write_fuses = p_operation.add_parser(
            "write-fuses", help="write and verify device fuses")
        p_write_fuses.add_argument(
            "-L", "--low", metavar="BITS", type=bits,
            help="set low fuse to binary BITS")
        p_write_fuses.add_argument(
            "-H", "--high", metavar="BITS", type=bits,
            help="set high fuse to binary BITS")
        p_write_fuses.add_argument(
            "-E", "--extra", metavar="BITS", type=bits,
            help="set extra fuse to binary BITS")

        p_write_lock = p_operation.add_parser(
            "write-lock", help="write and verify device lock bits")
        p_write_lock.add_argument(
            "bits", metavar="BITS", type=bits,
            help="write lock bits BITS")

        p_write_program = p_operation.add_parser(
            "write-program", help="write and verify device program memory")
        p_write_program.add_argument(
            "file", metavar="FILE",
            type=memory_file("program memory", argparse.FileType("rb")),
            help=f"read program memory contents from FILE {extension_help}")

        p_write_eeprom = p_operation.add_parser(
            "write-eeprom", help="write and verify device EEPROM")
        p_write_eeprom.add_argument(
            "file", metavar="FILE",
            type=memory_file("EEPROM", argparse.FileType("rb")),
            help=f"read EEPROM contents from FILE {extension_help}")

        p_erase = p_operation.add_parser(
            "erase", help="erase device lock bits, program memory, and EEPROM")

    async def interact(self, device, args, avr_iface):
        await avr_iface.programming_enable()

        try:
            signature = await avr_iface.read_signature()
            device = devices_by_signature[signature]
            self.logger.info("device signature: %s (%s)",
                "{:02x} {:02x} {:02x}".format(*signature),
                "unknown" if device is None else device.name)

            if args.operation in (None, "identify"):
                return

            if device is None:
                raise ProgramAVRError("cannot operate on unknown device")

            if device.erase_time is not None:
                avr_iface.erase_time = device.erase_time

            if args.operation == "read":
                if args.fuses:
                    fuses = await avr_iface.read_fuse_range(range(device.fuses_size))
                    if device.fuses_size > 2:
                        self.logger.info("fuses: low %s high %s extra %s",
                                         f"{fuses[0]:08b}",
                                         f"{fuses[1]:08b}",
                                         f"{fuses[2]:08b}")
                    elif device.fuses_size > 1:
                        self.logger.info("fuses: low %s high %s",
                                         f"{fuses[0]:08b}",
                                         f"{fuses[1]:08b}")
                    else:
                        self.logger.info("fuse: %s", f"{fuses[0]:08b}")

                if args.lock_bits:
                    lock_bits = await avr_iface.read_lock_bits()
                    self.logger.info("lock bits: %s", f"{lock_bits:08b}")

                if args.calibration:
                    calibration = \
                        await avr_iface.read_calibration_range(range(device.calibration_size))
                    self.logger.info("calibration bytes: %s",
                                     " ".join(["%02x" % b for b in calibration]))

                if args.program:
                    program_file, program_fmt = args.program
                    self.logger.info("reading program memory (%d bytes)", device.program_size)
                    output_data(program_file,
                        await avr_iface.read_program_memory_range(range(device.program_size)),
                        program_fmt)

                if args.eeprom:
                    eeprom_file, eeprom_fmt = args.eeprom
                    self.logger.info("reading EEPROM (%d bytes)", device.eeprom_size)
                    output_data(eeprom_file,
                        await avr_iface.read_eeprom_range(range(device.eeprom_size)),
                        eeprom_fmt)

            if args.operation == "write-fuses":
                if args.high and device.fuses_size < 2:
                    raise ProgramAVRError("device does not have high fuse")

                if args.low:
                    self.logger.info("writing low fuse")
                    await avr_iface.write_fuse(0, args.low)
                    written = await avr_iface.read_fuse(0)
                    if written != args.low:
                        raise ProgramAVRError("verification of low fuse failed: %s" %
                                              f"{written:08b} != {args.low:08b}")

                if args.high:
                    self.logger.info("writing high fuse")
                    await avr_iface.write_fuse(1, args.high)
                    written = await avr_iface.read_fuse(1)
                    if written != args.high:
                        raise ProgramAVRError("verification of high fuse failed: %s" %
                                              f"{written:08b} != {args.high:08b}")

                if args.extra:
                    self.logger.info("writing extra fuse")
                    await avr_iface.write_fuse(2, args.extra)
                    written = await avr_iface.read_fuse(2)
                    if written != args.extra:
                        raise ProgramAVRError("verification of extra fuse failed: %s" %
                                              f"{written:08b} != {args.extra:08b}")

            if args.operation == "write-lock":
                self.logger.info("writing lock bits")
                await avr_iface.write_lock_bits(args.bits)
                written = await avr_iface.read_lock_bits()
                if written != args.bits:
                    raise ProgramAVRError("verification of lock bits failed: %s" %
                                          f"{written:08b} != {args.bits:08b}")

            if args.operation == "write-program":
                self.logger.info("erasing chip")
                await avr_iface.chip_erase()

                program_file, program_fmt = args.file
                data = input_data(program_file, program_fmt)
                self.logger.info("writing program memory (%d bytes)",
                                 sum([len(chunk) for address, chunk in data]))
                for address, chunk in data:
                    chunk = bytes(chunk)
                    await avr_iface.write_program_memory_range(address, chunk, device.program_page)
                    written = await avr_iface.read_program_memory_range(range(address, address + len(chunk)))
                    if written != chunk:
                        raise ProgramAVRError("verification failed at address %#06x: %s != %s" %
                                              (address, written.hex(), chunk.hex()))

            if args.operation == "write-eeprom":
                eeprom_file, eeprom_fmt = args.file
                data = input_data(eeprom_file, eeprom_fmt)
                self.logger.info("writing EEPROM (%d bytes)",
                                 sum([len(chunk) for address, chunk in data]))
                for address, chunk in data:
                    chunk = bytes(chunk)
                    await avr_iface.write_eeprom_range(address, chunk, device.eeprom_page)
                    written = await avr_iface.read_eeprom_range(range(address, len(chunk)))
                    if written != chunk:
                        raise ProgramAVRError("verification failed at address %#06x: %s != %s" %
                                              (address, written.hex(), chunk.hex()))

            if args.operation == "erase":
                self.logger.info("erasing device")
                await avr_iface.chip_erase()
        finally:
            await avr_iface.programming_disable()
