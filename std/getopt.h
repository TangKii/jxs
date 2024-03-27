/***************************************************************************
** intoPIX SA & Fraunhofer IIS (hereinafter the "Software Copyright       **
** Holder") hold or have the right to license copyright with respect to   **
** the accompanying software (hereinafter the "Software").                **
**                                                                        **
** Copyright License for Evaluation and Testing                           **
** --------------------------------------------                           **
**                                                                        **
** The Software Copyright Holder hereby grants, to any implementer of     **
** this ISO Standard, an irrevocable, non-exclusive, worldwide,           **
** royalty-free, sub-licensable copyright licence to prepare derivative   **
** works of (including translations, adaptations, alterations), the       **
** Software and reproduce, display, distribute and execute the Software   **
** and derivative works thereof, for the following limited purposes: (i)  **
** to evaluate the Software and any derivative works thereof for          **
** inclusion in its implementation of this ISO Standard, and (ii)         **
** to determine whether its implementation conforms with this ISO         **
** Standard.                                                              **
**                                                                        **
** The Software Copyright Holder represents and warrants that, to the     **
** best of its knowledge, it has the necessary copyright rights to        **
** license the Software pursuant to the terms and conditions set forth in **
** this option.                                                           **
**                                                                        **
** No patent licence is granted, nor is a patent licensing commitment     **
** made, by implication, estoppel or otherwise.                           **
**                                                                        **
** Disclaimer: Other than as expressly provided herein, (1) the Software  **
** is provided “AS IS” WITH NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING  **
** BUT NOT LIMITED TO, THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A   **
** PARTICULAR PURPOSE AND NON-INFRINGMENT OF INTELLECTUAL PROPERTY RIGHTS **
** and (2) neither the Software Copyright Holder (or its affiliates) nor  **
** the ISO shall be held liable in any event for any damages whatsoever   **
** (including, without limitation, damages for loss of profits, business  **
** interruption, loss of information, or any other pecuniary loss)        **
** arising out of or related to the use of or inability to use the        **
** Software.”                                                             **
**                                                                        **
** RAND Copyright Licensing Commitment                                    **
** -----------------------------------                                    **
**                                                                        **
** IN THE EVENT YOU WISH TO INCLUDE THE SOFTWARE IN A CONFORMING          **
** IMPLEMENTATION OF THIS ISO STANDARD, PLEASE BE FURTHER ADVISED THAT:   **
**                                                                        **
** The Software Copyright Holder agrees to grant a copyright              **
** license on reasonable and non- discriminatory terms and conditions for **
** the purpose of including the Software in a conforming implementation   **
** of the ISO Standard. Negotiations with regard to the license are       **
** left to the parties concerned and are performed outside the ISO.       **
**                                                                        **
** No patent licence is granted, nor is a patent licensing commitment     **
** made, by implication, estoppel or otherwise.                           **
***************************************************************************/
 

#ifndef GETOPT_H
#define GETOPT_H

#include <string.h>
#include <stdio.h>

int opterr = 1,              
	optind = 1,              
	optopt,                  
	optreset;                
char *optarg;                 

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

 
int getopt(int nargc, char * const nargv[], const char *ostr)
{
	static char *place = EMSG;               
	const char *oli;                         

	if (optreset || !*place) {               
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return (-1);
		}
		if (place[1] && *++place == '-') {       
			++optind;
			place = EMSG;
			return (-1);
		}
	}                                        
	if ((optopt = (int)*place++) == (int)':' ||
		!(oli = strchr(ostr, optopt))) {
			 
			if (optopt == (int)'-')
				return (-1);
			if (!*place)
				++optind;
			if (opterr && *ostr != ':')
				(void)printf("illegal option -- %c\n", optopt);
			return (BADCH);
	}
	if (*++oli != ':') {                     
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {                                   
		if (*place)                      
			optarg = place;
		else if (nargc <= ++optind) {    
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)printf("option requires an argument -- %c\n", optopt);
			return (BADCH);
		}
		else                             
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt);                         
}


#endif
