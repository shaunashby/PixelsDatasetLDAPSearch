/****************************************************

$Id: genpixels_main.C,v 1.2 2008/06/11 14:13:27 oneel Exp $

  genpixels_main


        Developer : Bruce O'Neel
                    Integral Science Data Center

        Purpose   : This executable generates sky pixels from an image.

        List of function: 
        Revision History : 

$Log: genpixels_main.C,v $
Revision 2.0  2010/01/20  RR
Implementation of oversampling of the input images

Revision 1.2  2008/06/11 14:13:27  oneel
Cleanup, add @ to the input files to process a whole list of files,
and free memory

Revision 1.1  2008/01/07 19:24:23  oneel
Convert to C++ for much much better parameter checking.

Revision 1.6  2007/12/08 19:30:21  oneel
Compiles cleanly

Revision 1.5  2007/12/08 19:29:04  oneel
Snapshot for Mathis.

Revision 1.4  2007/06/26 15:36:00  oneel
More work, this time we have the loop over all images in the input
file, and then make the linked list (though don't read keywords yet)

Revision 1.3  2007/06/25 19:05:56  oneel
Skeleton

Revision 1.2  2007/06/25 18:48:25  oneel
Skeleton

Revision 1.1  2007/06/25 18:29:42  oneel
First version

****************************************************/

#include <genpixels.h>

#ifdef USE_LDAP_V3
#define HOST "ashby.isdc.unige.ch"
/* Parameters for searches: */
#define BASEDN "ou=Datasources,dc=ashby,dc=isdc,dc=unige,dc=ch"
#define SCOPE LDAP_SCOPE_SUBTREE

extern "C" {
#include <ldap.h>
  LDAP *ldap_init(char *host,int port);
  int ldap_simple_bind_s(LDAP *ld, const char *who, const char *passwd);
  int ldap_unbind_s(LDAP *ld);
  char **ldap_get_values(LDAP *ld, LDAPMessage *entry, char *attr);
  void ldap_value_free(char **vals);
}
#endif

int main (int argc, char **argv)
{

 int    status = ISDC_OK;
 int    mode = 0;
 char   inIMG[PIL_LINESIZE] = "";
 char   outputArea[PIL_LINESIZE] = "";
 char   referenceImage[PIL_LINESIZE] = "";
 
 int    overSampling;
 double lowL,highL,lowB,highB;
 FILE   *listFile;
 char   inFile[PIL_LINESIZE];
 

 /* Initialisation of the executable */
 mode = CommonInit(COMPONENT_NAME,COMPONENT_VERSION,argc,argv);
 if(mode != ISDC_SINGLE_MODE ){
   if(mode< ISDC_OK) status = mode;
   RILlogMessage(NULL,Error_1, "CommonInit not in SINGLE_MODE: mode = %d ", mode);
   mode=CommonExit(status);
 }

 /* input parameters */
 #ifndef USE_LDAP_V3
 /* Don't ask for image list if we're using LDAP: */
 if(status == ISDC_OK){ status = PILGetString("inIMG", inIMG);}
 #endif
 
 if(status == ISDC_OK){ status = PILGetReal("lowL",  &lowL);}
 if(status == ISDC_OK){ status = PILGetReal("highL", &highL);}
 if(status == ISDC_OK){ status = PILGetReal("lowB",  &lowB);}
 if(status == ISDC_OK){ status = PILGetReal("highB", &highB);}
 if(status == ISDC_OK){ status = PILGetString("outputArea", outputArea);}
 if(status == ISDC_OK){ status = PILGetString("referenceImage", referenceImage);}
 if(status == ISDC_OK){ status = PILGetInt("overSampling", &overSampling);}

#ifdef USE_LDAP_V3
 char LDAPQuery[PIL_LINESIZE] = ""; /* "(&(objectClass=integralDS)(cn=0780)(dsInstrument=isgri))" *
 /* For LDAP queries */
 if(status == ISDC_OK){ status = PILGetString("LDAPQuery",LDAPQuery);}
 
 if (LDAPQuery != "") {
   /* Use LDAP to get the list of image files */
   int ldp_result=0;
   LDAPMessage *result, *e;
   char *dn;
   char **imagefiles;
   
   RILlogMessage(NULL,Log_1, "Using LDAP search %s:%s to obtain image list.",HOST,LDAPQuery);

   /* Set up the connection to the LDAP server: */
   LDAP *handle;
   if ((handle = ldap_init(HOST, LDAP_PORT)) == NULL) {
     RILlogMessage(NULL,Error_1,"Can not init LDAP: call to ldap_init failed.");
     CommonExit(-100000);
   }

   /* Set the protocol version (V3): */
   int ldap_version = LDAP_VERSION3;
   ldap_set_option(handle, LDAP_OPT_PROTOCOL_VERSION, &ldap_version);	

   /* Bind to the server (anonymously): */
   ldp_result = ldap_simple_bind_s(handle,NULL,NULL);
   if ( ldp_result != LDAP_SUCCESS ) {
     RILlogMessage(NULL,Error_1,"Can not open connection to LDAP server %s: ldap_simple_bind_s: %s",HOST,ldap_err2string(ldp_result));
     CommonExit(-100000);
   }

   /* Run a query: */
   ldp_result = ldap_search_ext_s(handle,BASEDN,SCOPE,LDAPQuery, NULL, 0, NULL, NULL, NULL, 1000, &result);

   if ( ldp_result != LDAP_SUCCESS ) {
     RILlogMessage(NULL,Error_1,"LDAP search \"%s\", DN=\"%s\" failed: ldap_search_ext_s: %s",LDAPQuery,BASEDN,ldap_err2string(ldp_result));
     CommonExit(-100000);
   }
   
   int n_entries = ldap_count_entries(handle, result);

   RILlogMessage(NULL,Log_1, "Search using base DN \"%s\", filter \"%s\" successful. %d entries found.",BASEDN,LDAPQuery,n_entries);

   for ( e = ldap_first_entry(handle,result); e != NULL; e = ldap_next_entry(handle,e) ) {
     /* Print the matching DN (contains host info) */
     if ( (dn = ldap_get_dn(handle,e)) != NULL ) {
       RILlogMessage(NULL,Log_1, "LDAP: Got matching datasource DN - %s",dn);
       ldap_memfree(dn);
     }
     
     if ((imagefiles = ldap_get_values(handle, e, "dsimgPath")) != NULL ) {
       int i = 0;
       while (imagefiles[i]) {
	 RILlogMessage(NULL,Log_1, "LDAP: Got image file - %s",imagefiles[i]);
	 /* Call genpixels for each image file */
	 status = genpixels(imagefiles[i], outputArea,
			    referenceImage, lowL,highL,lowB,highB, overSampling, status);
	 i++;
       }
       RILlogMessage(NULL,Log_1, "LDAP: Search found %d dsimgPath entries." ,i);
     }
     /* Free memory for the imagefiles array */
     ldap_value_free(imagefiles);
   }

   /* Free memory for the result */
   ldap_msgfree(result);
  
   /* Unbind when finished: */
   ldp_result = ldap_unbind_s(handle);
   
 } else {
#endif
   /* do the work */
   if (inIMG[0] != '@') {
     status = genpixels(inIMG, outputArea, referenceImage,  
			lowL,highL,lowB,highB, overSampling, status);
   } else {
     listFile=fopen(inIMG+1,"r");
     if (!listFile) {
       RILlogMessage(NULL,Error_1,"Can not open %s",inIMG+1);
       CommonExit(-100000);
     }
     while (fgets(inFile,PIL_LINESIZE,listFile))
       {
	 inFile[strlen(inFile)-1] = '\0';
	 
	 status = genpixels(inFile, outputArea, 
			    referenceImage, lowL,highL,lowB,highB, overSampling, status); 
       }
     fclose(listFile);
   }
#ifdef USE_LDAP_V3
 }
#endif 
 mode=CommonExit(status);
}
