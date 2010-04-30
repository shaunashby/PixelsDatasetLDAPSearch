#include <stdio.h>
#include "ldap.h"

/* Server name: */
#define HOST "ashby.isdc.unige.ch"

/* Parameters for searches: */
#define BASEDN "ou=ISGRI,ou=Datasets,dc=ashby,dc=isdc,dc=unige,dc=ch"
#define SCOPE LDAP_SCOPE_SUBTREE
#define QUERY_FILTER "(&(objectClass=integralDS)(dsHost=compute-0-5))"

int main (int argc, const char * argv[]) {
	
	int ldp_result=0;
	LDAPMessage *result, *e;
	
	char *dn;
	
	/*Set up the connection to the LDAP server: */
	LDAP *handle;
	if ((handle = ldap_init(HOST, LDAP_PORT)) == NULL) {
		perror( "ldap_init" );
		return(1);
	}
		
	/* Set the protocol version: */
	int ldap_version = LDAP_VERSION3;
	ldap_set_option(handle, LDAP_OPT_PROTOCOL_VERSION, &ldap_version);	

	/* Bind to the server: */
	ldp_result = ldap_simple_bind_s(handle,NULL,NULL);
	if ( ldp_result != LDAP_SUCCESS ) {
		fprintf(stderr, "ldap_simple_bind_s: %s\n\n", ldap_err2string(ldp_result));
		return(1);
	} else {
		printf( "Bind to %s:%d as anonymous successful.\n",HOST,LDAP_PORT);
	}
	
	/* Run a query: */
	ldp_result = ldap_search_ext_s(handle,BASEDN,SCOPE,QUERY_FILTER, NULL, 0, NULL, NULL, NULL, 1000, &result);
	
	if ( ldp_result != LDAP_SUCCESS ) {
		fprintf(stderr, "ldap_search_ext_s: %s\n", ldap_err2string(ldp_result));
		return( 1 );
	} else {
		printf( "Search using base DN \"%s\", filter \"%s\" successful.\n",BASEDN,QUERY_FILTER);
	}
	
	for ( e = ldap_first_entry(handle,result); e != NULL; e = ldap_next_entry(handle,e) ) {
		if ( (dn = ldap_get_dn(handle,e)) != NULL ) {
			printf("dn: %s\n", dn);
			ldap_memfree(dn);
		}
	}
	
	ldap_msgfree( result );
	
	/* Unbind when finished: */
	ldp_result = ldap_unbind_s(handle);
	return 0;
}
