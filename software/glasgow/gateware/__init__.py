import functools
from amaranth import Elaboratable
from amaranth.sim import Simulator


__all__ = ["simulation_test"]


def simulation_test(case=None, testbench=False, **kwargs):
    def configure_wrapper(case):
        @functools.wraps(case)
        def wrapper(self):
            if hasattr(self, "configure"):
                self.configure(self.tb, **kwargs)
            def setup_wrapper():
                if hasattr(self, "simulationSetUp"):
                    yield from self.simulationSetUp(self.tb)
                yield from case(self, self.tb)
            if isinstance(self.tb, Elaboratable):
                sim = Simulator(self.tb)
                with sim.write_vcd("test.vcd"):
                    sim.add_clock(1e-8)
                    if testbench:
                        sim.add_testbench(setup_wrapper)
                    else:
                        sim.add_sync_process(setup_wrapper)
                    sim.run()
        return wrapper

    if case is None:
        return configure_wrapper
    else:
        return configure_wrapper(case)
