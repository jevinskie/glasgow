from amaranth import *


__all__ = ["Registers", "I2CRegisters"]


class Registers(Elaboratable):
    """
    A register array.

    :attr reg_count:
        Register count.
    """
    def __init__(self):
        self.reg_count = 0
        self.regs_r = Array()
        self.regs_w = Array()
        self.regs_rst = []

    def _add_reg(self, *args, **kwargs):
        reg  = Signal(*args, **kwargs, src_loc_at=2)
        addr = self.reg_count
        self.reg_count += 1
        return reg, addr

    def add_ro(self, *args, **kwargs):
        reg, addr = self._add_reg(*args, **kwargs)
        self.regs_r.append(reg)
        self.regs_w.append(Signal(name="ro_reg_dummy"))
        return reg, addr

    def add_rw(self, *args, domain=None, **kwargs):
        reg, addr = self._add_reg(*args, **kwargs)
        self.regs_r.append(reg)
        self.regs_w.append(reg)
        if domain is not None and domain.rst is not None and not reg.reset_less:
            self.regs_rst.append((reg, domain.rst))
        return reg, addr

    def add_existing_ro(self, reg):
        addr = self.reg_count
        self.reg_count += 1
        self.regs_r.append(reg)
        self.regs_w.append(Signal(name="ro_reg_dummy"))
        return addr

    def add_existing_rw(self, reg, *, domain=None):
        addr = self.reg_count
        self.reg_count += 1
        self.regs_r.append(reg)
        self.regs_w.append(reg)
        if domain is not None and domain.rst is not None and not reg.reset_less:
            self.regs_rst.append((reg, domain.rst))
        return addr

    def elaborate(self, platform):
        m = Module()
        return m


class I2CRegisters(Registers):
    """
    A register array, accessible over I2C.

    Note that for multibyte registers, the register data is read in little endian, but written
    in big endian. This replaces a huge multiplexer with a shift register, but is a bit cursed.
    """
    def __init__(self, i2c_target):
        super().__init__()
        self.i2c_target = i2c_target

    def elaborate(self, platform):
        m = super().elaborate(platform)

        if self.reg_count != 0:
            latch_addr = Signal()
            reg_addr   = Signal(range(self.reg_count))
            reg_data   = Signal(max(len(Value.cast(s)) for s in self.regs_r))
            reg_update = Signal()

            m.d.comb += self.i2c_target.data_o.eq(reg_data)

            with m.If(self.i2c_target.start):
                m.d.sync += latch_addr.eq(1)
                m.d.sync += reg_update.eq(0)

            with m.If(self.i2c_target.write):
                m.d.sync += latch_addr.eq(0)

                with m.If(latch_addr):
                    with m.If(self.i2c_target.data_i < self.reg_count):
                        m.d.comb += self.i2c_target.ack_o.eq(1)
                    m.d.sync += reg_addr.eq(self.i2c_target.data_i)
                    m.d.sync += reg_data.eq(self.regs_r[self.i2c_target.data_i])

                with m.Else():
                    m.d.comb += self.i2c_target.ack_o.eq(1)
                    m.d.sync += reg_data.eq(Cat(self.i2c_target.data_i, reg_data))
                    m.d.sync += reg_update.eq(1)

            with m.If(self.i2c_target.read):
                m.d.sync += reg_data.eq(reg_data >> 8)

            with m.If(self.i2c_target.stop):
                with m.If(reg_update):
                    m.d.sync += self.regs_w[reg_addr].eq(reg_data)

            for reg, reg_rst in self.regs_rst:
                with m.If(reg_rst):
                    m.d.sync += reg.eq(reg.init)

        return m
