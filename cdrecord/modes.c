/* @(#)modes.c	1.13 99/06/06 Copyright 1988 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)modes.c	1.13 99/06/06 Copyright 1988 J. Schilling";
#endif
/*
 *	SCSI mode page handling
 *
 *	Copyright (c) 1988 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mconfig.h>
#include <sys/types.h>
#include <standard.h>
#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"

EXPORT int	scsi_compliant;

LOCAL	BOOL	has_mode_page	__PR((SCSI *scgp, int page, char *pagename, int *lenp));
EXPORT	BOOL	get_mode_params	__PR((SCSI *scgp, int page, char *pagename,
					u_char *modep, u_char *cmodep,
					u_char *dmodep, u_char *smodep,
					int *lenp));
EXPORT	BOOL	set_mode_params	__PR((SCSI *scgp, char *pagename, u_char *modep,
					int len, int save, int secsize));

#define	XXX

#ifdef	XXX
LOCAL
BOOL has_mode_page(scgp, page, pagename, lenp)
	SCSI	*scgp;
	int	page;
	char	*pagename;
	int	*lenp;
{
	u_char	mode[0x100];
	int	hdlen;
	int	len = 1;				/* Nach SCSI Norm */
	int	try = 0;
	struct	scsi_mode_page_header *mp;

again:
	fillbytes((caddr_t)mode, sizeof(mode), '\0');
	if (lenp)
		*lenp = 0;

	(void)test_unit_ready(scgp);
	scgp->silent++;
/* Maxoptix bringt Aborted cmd 0x0B mit code 0x4E (overlapping cmds)*/

	/*
	 * The Matsushita CW-7502 will sometimes deliver a zeroed
	 * mode page 2A if "Page n default" is used instead of "current".
	 */
	if (mode_sense(scgp, mode, len, page, 0) < 0) {	/* Page n current */
		scgp->silent--;
		return (FALSE);
	} else {
		len = ((struct scsi_mode_header *)mode)->sense_data_len + 1;
	}
	if (mode_sense(scgp, mode, len, page, 0) < 0) {	/* Page n current */
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;

	if (scgp->verbose)
		scsiprbytes("Mode Sense Data", mode, len - scsigetresid(scgp));
	hdlen = sizeof(struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len;
	mp = (struct scsi_mode_page_header *)(mode + hdlen);
	if (scgp->verbose)
		scsiprbytes("Mode Page  Data", (u_char *)mp, mp->p_len+2);

	if (mp->p_len == 0) {
		if (!scsi_compliant && try == 0) {
			len = hdlen;
			/*
			 * add sizeof page header (page # + len byte)
			 * (should normaly result in len == 14)
			 * this allowes to work with:
			 * 	Quantum Q210S	(wants at least 13)
			 * 	MD2x		(wants at least 4)
			 */
			len += 2;
			try++;
			goto again;
		}
		/* XXX if (!nowarn) */
		errmsgno(EX_BAD,
			"Warning: controller returns zero sized %s page.\n",
								pagename);
	}
	if (!scsi_compliant &&
	    (len < (int)(mp->p_len + hdlen + 2))) {
		len = mp->p_len + hdlen + 2;

		/* XXX if (!nowarn) */
		errmsgno(EX_BAD,
			"Warning: controller returns wrong size for %s page.\n",
								pagename);
	}
	if (mp->p_code != page) {
		/* XXX if (!nowarn) */
		errmsgno(EX_BAD,
			"Warning: controller returns wrong page %X for %s page (%X).\n",
						mp->p_code, pagename, page);
		return (FALSE);
	}

	if (lenp)
		*lenp = len;
	return (mp->p_len > 0);
}
#endif

EXPORT
BOOL get_mode_params(scgp, page, pagename, modep, cmodep, dmodep, smodep, lenp)
	SCSI	*scgp;
	int	page;
	char	*pagename;
	u_char	*modep;
	u_char	*cmodep;
	u_char	*dmodep;
	u_char	*smodep;
	int	*lenp;
{
	int	len;
	BOOL	ret = TRUE;

#ifdef	XXX
	if (lenp)
		*lenp = 0;
	if (!has_mode_page(scgp, page, pagename, &len)) {
		if (!scgp->silent) errmsgno(EX_BAD,
			"Warning: controller does not support %s page.\n",
								pagename);
		return (FALSE);
	}
	if (lenp)
		*lenp = len;
#else
	if (lenp == 0)
		len = 0xFF;
#endif

	if (modep) {
		fillbytes(modep, 0x100, '\0');
		if (mode_sense(scgp, modep, len, page, 0) < 0) {/* Page x current */
			errmsgno(EX_BAD, "Cannot get %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			scsiprbytes("Mode Sense Data", modep, len - scsigetresid(scgp));
		}
	}

	if (cmodep) {
		fillbytes(cmodep, 0x100, '\0');
		if (mode_sense(scgp, cmodep, len, page, 1) < 0) {/* Page x change */
			errmsgno(EX_BAD, "Cannot get %s mask.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			scsiprbytes("Mode Sense Data", cmodep, len - scsigetresid(scgp));
		}
	}

	if (dmodep) {
		fillbytes(dmodep, 0x100, '\0');
		if (mode_sense(scgp, dmodep, len, page, 2) < 0) {/* Page x default */
			errmsgno(EX_BAD, "Cannot get default %s data.\n",
								pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			scsiprbytes("Mode Sense Data", dmodep, len - scsigetresid(scgp));
		}
	}

	if (smodep) {
		fillbytes(smodep, 0x100, '\0');
		if (mode_sense(scgp, smodep, len, page, 3) < 0) {/* Page x saved */
			errmsgno(EX_BAD, "Cannot get saved %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			scsiprbytes("Mode Sense Data", smodep, len - scsigetresid(scgp));
		}
	}

	return (ret);
}

EXPORT
BOOL set_mode_params(scgp, pagename, modep, len, save, secsize)
	SCSI	*scgp;
	char	*pagename;
	u_char	*modep;
	int	len;
	int	save;
	int	secsize;
{
	int	i;

	((struct scsi_modesel_header *)modep)->sense_data_len	= 0;
	((struct scsi_modesel_header *)modep)->res2		= 0;

	i = ((struct scsi_mode_header *)modep)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)modep)->blockdesc.nlblock,
								0);
		if (secsize >= 0)
		i_to_3_byte(((struct scsi_mode_data *)modep)->blockdesc.lblen,
							secsize);
	}

	if (save == 0 || mode_select(scgp, modep, len, save, scgp->inq->data_format >= 2) < 0) {
		if (mode_select(scgp, modep, len, 0, scgp->inq->data_format >= 2) < 0) {
			errmsgno(EX_BAD,
			   "Warning: using default %s data.\n", pagename);
			scsiprbytes("Mode Select Data", modep, len);
			return (FALSE);
		}
	}
	return (TRUE);
}
