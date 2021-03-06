// license:BSD-3-Clause
// copyright-holders:Curt Coder, Robbbert
/********************************************************************************************************************

2013-07-31 Skeleton Driver [Curt Coder]
2013-07-31 Connected to terminal [Robbbert]
2016-07-11 After 10 seconds the monitor program will start [Robbbert]

Commands: (no spaces allowed)
B - Boot the disk
D - Dump memory to screen
F - Fill Memory
G - Go To
H - Help
P - Alter port values
S - Alter memory


The photos show 3 boards:
- A scsi board (all 74-series TTL)
- CPU board (64k dynamic RAM, Z80A CPU, 2x Z80CTC, 2x Z80SIO/0, MB8877A, Z80DMA, 4x MC1488,
  4x MC1489, XTALS 1.8432MHz and 24MHz)
- ADES board (Adaptec Inc AIC-100, AIC-250, AIC-300, Intel D8086AH, unknown crystal)

Both roms contain Z80 code.


********************************************************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80daisy.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/clock.h"
#include "bus/rs232/rs232.h"


class dsb46_state : public driver_device
{
public:
	dsb46_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
	{ }

	void dsb46(machine_config &config);

	void init_dsb46();

private:
	DECLARE_WRITE8_MEMBER(port1a_w);
	DECLARE_MACHINE_RESET(dsb46);
	void dsb46_io(address_map &map);
	void dsb46_mem(address_map &map);
	required_device<cpu_device> m_maincpu;
};

void dsb46_state::dsb46_mem(address_map &map)
{
	map(0x0000, 0x07ff).bankr("read").bankw("write");
	map(0x0800, 0xffff).ram();
}

void dsb46_state::dsb46_io(address_map &map)
{
	map.global_mask(0xff);
	map.unmap_value_high();
	map(0x00, 0x03).rw("sio", FUNC(z80sio_device::ba_cd_r), FUNC(z80sio_device::ba_cd_w));
	map(0x08, 0x0b).rw("ctc1", FUNC(z80ctc_device::read), FUNC(z80ctc_device::write));
	map(0x1a, 0x1a).w(FUNC(dsb46_state::port1a_w));
	//AM_RANGE(0x10, 0x10) disk related
	//AM_RANGE(0x14, 0x14) ?? (read after CTC1 TRG3)
	//AM_RANGE(0x18, 0x18) ??
	//AM_RANGE(0x1c, 0x1c) disk data
	//AM_RANGE(0x1d, 0x1d) disk status (FF = no fdc)
}

static INPUT_PORTS_START( dsb46 )
INPUT_PORTS_END

void dsb46_state::init_dsb46()
{
	uint8_t *RAM = memregion("maincpu")->base();
	membank("read")->configure_entry(0, &RAM[0x10000]);
	membank("read")->configure_entry(1, &RAM[0x00000]);
	membank("write")->configure_entry(0, &RAM[0x00000]);
}

MACHINE_RESET_MEMBER( dsb46_state,dsb46 )
{
	membank("read")->set_entry(0);
	membank("write")->set_entry(0);
	m_maincpu->reset();
}

WRITE8_MEMBER( dsb46_state::port1a_w )
{
	membank("read")->set_entry(data & 1);
}

static const z80_daisy_config daisy_chain[] =
{
	{ "ctc1" },
	{ "sio" },
	{ nullptr }
};


MACHINE_CONFIG_START(dsb46_state::dsb46)
	// basic machine hardware
	MCFG_DEVICE_ADD("maincpu", Z80, XTAL(24'000'000) / 6)
	MCFG_DEVICE_PROGRAM_MAP(dsb46_mem)
	MCFG_DEVICE_IO_MAP(dsb46_io)
	MCFG_Z80_DAISY_CHAIN(daisy_chain)

	MCFG_MACHINE_RESET_OVERRIDE(dsb46_state, dsb46)

	/* video hardware */
	clock_device &ctc_clock(CLOCK(config, "ctc_clock", 1.8432_MHz_XTAL));
	ctc_clock.signal_handler().set("ctc1", FUNC(z80ctc_device::trg0));
	ctc_clock.signal_handler().append("ctc1", FUNC(z80ctc_device::trg2));

	/* Devices */
	MCFG_DEVICE_ADD("sio", Z80SIO, XTAL(24'000'000) / 6)
	MCFG_Z80SIO_OUT_INT_CB(INPUTLINE("maincpu", INPUT_LINE_IRQ0))
	MCFG_Z80SIO_OUT_TXDA_CB(WRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_Z80SIO_OUT_DTRA_CB(WRITELINE("rs232", rs232_port_device, write_dtr))
	MCFG_Z80SIO_OUT_RTSA_CB(WRITELINE("rs232", rs232_port_device, write_rts))

	MCFG_DEVICE_ADD("rs232", RS232_PORT, default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(WRITELINE("sio", z80sio_device, rxa_w))
	MCFG_RS232_CTS_HANDLER(WRITELINE("sio", z80sio_device, ctsa_w))

	z80ctc_device &ctc1(Z80CTC(config, "ctc1", 24_MHz_XTAL / 6));
	ctc1.intr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
	ctc1.zc_callback<0>().set("sio", FUNC(z80sio_device::rxca_w));
	ctc1.zc_callback<0>().append("sio", FUNC(z80sio_device::txca_w));
	ctc1.zc_callback<2>().set("sio", FUNC(z80sio_device::rxcb_w));
	ctc1.zc_callback<2>().append("sio", FUNC(z80sio_device::txcb_w));
MACHINE_CONFIG_END

ROM_START( dsb46 )
	ROM_REGION( 0x10800, "maincpu", 0 )
	ROM_LOAD( "1538a.bin", 0x10000, 0x800, CRC(65b3e26e) SHA1(afe1f03f266b7d13fdb1f1bc6762df5e0aa5c764) )

	ROM_REGION( 0x4000, "ades", 0 )
	ROM_LOAD( "ades.bin", 0x0000, 0x4000, CRC(d374abf0) SHA1(331f51a2bb81375aeffbe63c1ebc1d7cd779b9c3) )
ROM_END

COMP( 198?, dsb46, 0, 0, dsb46, dsb46, dsb46_state, init_dsb46, "Davidge", "DSB-4/6",  MACHINE_NOT_WORKING | MACHINE_NO_SOUND_HW )
