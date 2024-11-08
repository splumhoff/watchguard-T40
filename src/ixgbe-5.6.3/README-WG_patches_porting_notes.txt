
Notes for porting WG patches from //tps/ixgbe/5.0.5/mainline/src/... to current ixgbe v5.6.3 driver
---------------------------------------------------------------------------------------------------

1) For cL # 557340 (FBX-9879 M270 Hw sw port link issue).
   The api John's patch to poll the PHY regs is dropped in ixgbe v5.6.3 driver. The best way is to add those subroutines back after going over the patch. We might still re-visit the patch about the max mtu settings later.

2) CL 560803 not ported
   It was going to temporarily revert the change for CL 560794 (use v4.1.5 driver instead). It's confirmed that we still need CL # 560794 (fix for FBX-11388) for our v5.6.3 driver.

3) CL 574658 is not ported
   According to the description of CL 574658: Root Cause (Bug) or Purpose (RFE/Task): Intel drivers do not build under 4.14.
   The existing latest v5.6.3 already compiled fine with our 4.14 kernel without CL 574658. So we don't need CL 574658

