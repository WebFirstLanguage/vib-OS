/*
 * vib-OS Installer Application
 *
 * GUI installer for installing vib-OS to disk
 */

#ifndef INSTALLER_H
#define INSTALLER_H

/* Initialize and show installer window */
void installer_init(void);

/* Check if we're in live boot mode */
int installer_should_show(void);

#endif /* INSTALLER_H */
