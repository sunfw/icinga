/***********************************************************************
 *
 * CONFIG.C - Icinga Configuration CGI (View Only)
 *
 * Copyright (c) 1999-2009 Ethan Galstad (egalstad@nagios.org)
 * Copyright (c) 2009-2011 Icinga Development Team (http://www.icinga.org)
 *
 * This CGI program will display various configuration information.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ***********************************************************************/

#include "../include/config.h"
#include "../include/common.h"
#include "../include/objects.h"
#include "../include/macros.h"
#include "../include/cgiutils.h"
#include "../include/cgiauth.h"
#include "../include/getcgi.h"

static icinga_macros *mac;

extern char   main_config_file[MAX_FILENAME_LENGTH];
extern char   url_html_path[MAX_FILENAME_LENGTH];
extern char   url_docs_path[MAX_FILENAME_LENGTH];
extern char   url_images_path[MAX_FILENAME_LENGTH];
extern char   url_logo_images_path[MAX_FILENAME_LENGTH];
extern char   url_stylesheets_path[MAX_FILENAME_LENGTH];
extern char   url_js_path[MAX_FILENAME_LENGTH];

extern host *host_list;
extern service *service_list;
extern hostgroup *hostgroup_list;
extern servicegroup *servicegroup_list;
extern contactgroup *contactgroup_list;
extern command *command_list;
extern timeperiod *timeperiod_list;
extern contact *contact_list;
extern servicedependency *servicedependency_list;
extern serviceescalation *serviceescalation_list;
extern hostdependency *hostdependency_list;
extern hostescalation *hostescalation_list;
extern module *module_list;

extern int display_header;
extern int content_type;

extern char *csv_delimiter;
extern char *csv_data_enclosure;

int process_cgivars(void);

void display_options(void);

void display_hosts(void);
void display_hostgroups(void);
void display_servicegroups(void);
void display_contacts(void);
void display_contactgroups(void);
void display_services(void);
void display_timeperiods(void);
void display_commands(void);
void display_servicedependencies(void);
void display_serviceescalations(void);
void display_hostdependencies(void);
void display_hostescalations(void);
void display_command_expansion(void);
void display_modules(void);

authdata current_authdata;

int display_type = DISPLAY_NONE;
int show_all_hosts = TRUE;
int show_all_hostgroups = TRUE;
int show_all_servicegroups = TRUE;
char *host_name = NULL;
char *host_filter = NULL;
char *hostgroup_name = NULL;
char *servicegroup_name = NULL;
char *service_desc = NULL;
char *service_filter = NULL;
char to_expand[MAX_COMMAND_BUFFER];
char hashed_color[8];

extern int embedded;
extern int daemon_check;

extern char resource_file[MAX_INPUT_BUFFER];
extern char *macro_user[MAX_USER_MACROS];

int CGI_ID = CONFIG_CGI_ID;

int main(void) {
	int result = OK;
	char *seldesc = "";
	mac = get_global_macros();

	/* get the arguments passed in the URL */
	process_cgivars();

	/* reset internal variables */
	reset_cgi_vars();

	/* read the CGI configuration file */
	result = read_cgi_config_file(get_cgi_config_location());
	if (result == ERROR) {
		document_header(CGI_ID, FALSE);
		print_error(get_cgi_config_location(), ERROR_CGI_CFG_FILE);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read the main configuration file */
	result = read_main_config_file(main_config_file);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE);
		print_error(main_config_file, ERROR_CGI_MAIN_CFG);
		document_footer(CGI_ID);
		return ERROR;
	}

	/* read all object configuration data */
	result = read_all_object_configuration_data(main_config_file, READ_ALL_OBJECT_DATA);
	if (result == ERROR) {
		document_header(CGI_ID, FALSE);
		print_error(NULL, ERROR_CGI_OBJECT_DATA);
		document_footer(CGI_ID);
		return ERROR;
	}

        /* read resource file if possible. if not, ignore error at this time */
        read_icinga_resource_file(resource_file);

	/* initialize macros */
	init_macros();

	document_header(CGI_ID, TRUE);

	/* get authentication information */
	get_authentication_information(&current_authdata);

	/* see if user is authorized to view contact information... */
	if (is_authorized_for_configuration_information(&current_authdata) == FALSE) {
		print_generic_error_message("It appears as though you do not have permission to view the configuration information you requested...", "If you believe this is an error, check the HTTP server authentication requirements for accessing this CGI and check the authorization options in your CGI configuration file.", 0);
		document_footer(CGI_ID);
		return OK;
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		/* begin top table */
		printf("<table border=0 width=100%%>\n");
		printf("<tr>\n");

		/* left column of the first row */
		printf("<td align=left valign=top width=50%%>\n");
		display_info_table("Configuration", FALSE, &current_authdata, daemon_check);
		printf("</td>\n");

		/* right hand column of top row */
		printf("<td align=right valign=bottom width=50%%>\n");

		if (display_type != DISPLAY_NONE) {

			printf("<table border=0>\n");
			printf("<form method=\"get\" action=\"%s\">\n", CONFIG_CGI);

			printf("<tr><td align=left class='reportSelectSubTitle'>Object Type:</td></tr>\n");
			printf("<tr><td align=left class='reportSelectItem'>");
			printf("<select name='type'>\n");
			printf("<option value='hosts' %s>Hosts\n", (display_type == DISPLAY_HOSTS) ? "SELECTED" : "");
			printf("<option value='hostdependencies' %s>Host Dependencies\n", (display_type == DISPLAY_HOSTDEPENDENCIES) ? "SELECTED" : "");
			printf("<option value='hostescalations' %s>Host Escalations\n", (display_type == DISPLAY_HOSTESCALATIONS) ? "SELECTED" : "");
			printf("<option value='hostgroups' %s>Host Groups\n", (display_type == DISPLAY_HOSTGROUPS) ? "SELECTED" : "");
			printf("<option value='services' %s>Services\n", (display_type == DISPLAY_SERVICES) ? "SELECTED" : "");
			printf("<option value='servicegroups' %s>Service Groups\n", (display_type == DISPLAY_SERVICEGROUPS) ? "SELECTED" : "");
			printf("<option value='servicedependencies' %s>Service Dependencies\n", (display_type == DISPLAY_SERVICEDEPENDENCIES) ? "SELECTED" : "");
			printf("<option value='serviceescalations' %s>Service Escalations\n", (display_type == DISPLAY_SERVICEESCALATIONS) ? "SELECTED" : "");
			printf("<option value='contacts' %s>Contacts\n", (display_type == DISPLAY_CONTACTS) ? "SELECTED" : "");
			printf("<option value='contactgroups' %s>Contact Groups\n", (display_type == DISPLAY_CONTACTGROUPS) ? "SELECTED" : "");
			printf("<option value='timeperiods' %s>Timeperiods\n", (display_type == DISPLAY_TIMEPERIODS) ? "SELECTED" : "");
			printf("<option value='modules' %s>Modules\n", (display_type == DISPLAY_MODULES) ? "SELECTED" : "");
			printf("<option value='commands' %s>Commands\n", (display_type == DISPLAY_COMMANDS) ? "SELECTED" : "");
			printf("<option value='command' %s>Command Expansion\n", (display_type == DISPLAY_COMMAND_EXPANSION) ? "SELECTED" : "");
			printf("</select>\n");
			printf("</td></tr>\n");

			if (display_type != DISPLAY_COMMAND_EXPANSION) {
				if (display_type == DISPLAY_SERVICES) {
					seldesc = " Services Named or on Host";
				} else if (display_type == DISPLAY_SERVICEDEPENDENCIES) {
					seldesc = " Dependencies with Host";
				} else if (display_type == DISPLAY_SERVICEESCALATIONS) {
					seldesc = " Escalations on Host";
				} else if (display_type == DISPLAY_HOSTDEPENDENCIES) {
					seldesc = " Dependencies on/of Host";
				} else if (display_type == DISPLAY_HOSTESCALATIONS) {
					seldesc = " Escalations for Host";
				}
				printf("<tr><td align=left class='reportSelectSubTitle'>Show Only%s:</td></tr>\n", seldesc);
				printf("<tr><td align=left class='reportSelectItem'><input type='text' name='expand'\n");
				printf("value='%s'>", escape_string(to_expand));
			}

			printf("<tr><td class='reportSelectItem'><input type='submit' value='Update'></td></tr>\n");
			printf("</form>\n");
			printf("</table>\n");

			printf("<div class='csv_export_link'>");
			if (display_type != DISPLAY_COMMAND_EXPANSION) {
				print_export_link(CSV_CONTENT, CONFIG_CGI, NULL);
				print_export_link(JSON_CONTENT, CONFIG_CGI, NULL);
			}
			print_export_link(HTML_CONTENT, CONFIG_CGI, NULL);
			printf("</div>\n");
		}
	}

	/* display context-sensitive help */
	switch (display_type) {
	case DISPLAY_HOSTS:
		display_context_help(CONTEXTHELP_CONFIG_HOSTS);
		break;
	case DISPLAY_HOSTGROUPS:
		display_context_help(CONTEXTHELP_CONFIG_HOSTGROUPS);
		break;
	case DISPLAY_SERVICEGROUPS:
		display_context_help(CONTEXTHELP_CONFIG_SERVICEGROUPS);
		break;
	case DISPLAY_CONTACTS:
		display_context_help(CONTEXTHELP_CONFIG_CONTACTS);
		break;
	case DISPLAY_CONTACTGROUPS:
		display_context_help(CONTEXTHELP_CONFIG_CONTACTGROUPS);
		break;
	case DISPLAY_SERVICES:
		display_context_help(CONTEXTHELP_CONFIG_SERVICES);
		break;
	case DISPLAY_TIMEPERIODS:
		display_context_help(CONTEXTHELP_CONFIG_TIMEPERIODS);
		break;
	case DISPLAY_COMMANDS:
		display_context_help(CONTEXTHELP_CONFIG_COMMANDS);
		break;
	case DISPLAY_SERVICEDEPENDENCIES:
		display_context_help(CONTEXTHELP_CONFIG_SERVICEDEPENDENCIES);
		break;
	case DISPLAY_SERVICEESCALATIONS:
		display_context_help(CONTEXTHELP_CONFIG_HOSTESCALATIONS);
		break;
	case DISPLAY_HOSTDEPENDENCIES:
		display_context_help(CONTEXTHELP_CONFIG_HOSTDEPENDENCIES);
		break;
	case DISPLAY_HOSTESCALATIONS:
		display_context_help(CONTEXTHELP_CONFIG_HOSTESCALATIONS);
		break;
	case DISPLAY_COMMAND_EXPANSION:
		/* Reusing DISPLAY_COMMANDS help until further notice */
		display_context_help(CONTEXTHELP_CONFIG_COMMANDS);
		break;
	case DISPLAY_MODULES:
		/* reuse commands context help */
		display_context_help(CONTEXTHELP_CONFIG_COMMANDS);
		break;
	case DISPLAY_ALL:
		break;
	default:
		display_context_help(CONTEXTHELP_CONFIG_MENU);
		break;
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</td>\n");

		/* end of top table */
		printf("</tr>\n");
		printf("</table>\n");
	}


	switch (display_type) {
	case DISPLAY_HOSTS:
		display_hosts();
		break;
	case DISPLAY_HOSTGROUPS:
		display_hostgroups();
		break;
	case DISPLAY_SERVICEGROUPS:
		display_servicegroups();
		break;
	case DISPLAY_CONTACTS:
		display_contacts();
		break;
	case DISPLAY_CONTACTGROUPS:
		display_contactgroups();
		break;
	case DISPLAY_SERVICES:
		display_services();
		break;
	case DISPLAY_TIMEPERIODS:
		display_timeperiods();
		break;
	case DISPLAY_COMMANDS:
		display_commands();
		break;
	case DISPLAY_SERVICEDEPENDENCIES:
		display_servicedependencies();
		break;
	case DISPLAY_SERVICEESCALATIONS:
		display_serviceescalations();
		break;
	case DISPLAY_HOSTDEPENDENCIES:
		display_hostdependencies();
		break;
	case DISPLAY_HOSTESCALATIONS:
		display_hostescalations();
		break;
	case DISPLAY_COMMAND_EXPANSION:
		display_command_expansion();
		break;
	case DISPLAY_MODULES:
		display_modules();
		break;
	case DISPLAY_ALL:
		if (content_type == JSON_CONTENT) {
			display_hosts();
			printf(",\n");
			display_hostgroups();
			printf(",\n");
			display_servicegroups();
			printf(",\n");
			display_contacts();
			printf(",\n");
			display_contactgroups();
			printf(",\n");
			display_services();
			printf(",\n");
			display_timeperiods();
			printf(",\n");
			display_commands();
			printf(",\n");
			display_servicedependencies();
			printf(",\n");
			display_serviceescalations();
			printf(",\n");
			display_hostdependencies();
			printf(",\n");
			display_hostescalations();
			printf(",\n");
			display_modules();
			break;
		}
	default:
		display_options();
		break;
	}

	document_footer(CGI_ID);

	return OK;
}

int process_cgivars(void) {
	char **variables;
	int error = FALSE;
	int x;

	variables = getcgivars();
	to_expand[0] = '\0';

	for (x = 0; variables[x] != NULL; x++) {

		/* do some basic length checking on the variable identifier to prevent buffer overflows */
		if (strlen(variables[x]) >= MAX_INPUT_BUFFER - 1) {
			x++;
			continue;
		}

		/* we found the host name */
                else if (!strcmp(variables[x], "host")) {
                        x++;
                        if (variables[x] == NULL) {
                                error=TRUE;
                                break;
                        }

                        host_name = strdup(variables[x]);
                        if(host_name == NULL)
                                host_name = "";
                        strip_html_brackets(host_name);
                }

                /* we found the service name */
                else if (!strcmp(variables[x], "service")) {
                        x++;
                        if (variables[x] == NULL) {
                                error=TRUE;
                                break;
                        }

                        service_desc = strdup(variables[x]);
                        if(service_desc == NULL)
                                service_desc = "";
                        strip_html_brackets(service_desc);
                }

		/* we found the configuration type argument */
		else if (!strcmp(variables[x], "type")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}

			/* what information should we display? */
			if (!strcmp(variables[x], "hosts"))
				display_type = DISPLAY_HOSTS;
			else if (!strcmp(variables[x], "hostgroups"))
				display_type = DISPLAY_HOSTGROUPS;
			else if (!strcmp(variables[x], "servicegroups"))
				display_type = DISPLAY_SERVICEGROUPS;
			else if (!strcmp(variables[x], "contacts"))
				display_type = DISPLAY_CONTACTS;
			else if (!strcmp(variables[x], "contactgroups"))
				display_type = DISPLAY_CONTACTGROUPS;
			else if (!strcmp(variables[x], "services"))
				display_type = DISPLAY_SERVICES;
			else if (!strcmp(variables[x], "timeperiods"))
				display_type = DISPLAY_TIMEPERIODS;
			else if (!strcmp(variables[x], "commands"))
				display_type = DISPLAY_COMMANDS;
			else if (!strcmp(variables[x], "servicedependencies"))
				display_type = DISPLAY_SERVICEDEPENDENCIES;
			else if (!strcmp(variables[x], "serviceescalations"))
				display_type = DISPLAY_SERVICEESCALATIONS;
			else if (!strcmp(variables[x], "hostdependencies"))
				display_type = DISPLAY_HOSTDEPENDENCIES;
			else if (!strcmp(variables[x], "hostescalations"))
				display_type = DISPLAY_HOSTESCALATIONS;
			else if (!strcmp(variables[x], "command"))
				display_type = DISPLAY_COMMAND_EXPANSION;
			else if (!strcmp(variables[x], "modules"))
				display_type = DISPLAY_MODULES;
			else if (!strcmp(variables[x], "all"))
				display_type = DISPLAY_ALL;

			/* we found the embed option */
			else if (!strcmp(variables[x], "embedded"))
				embedded = TRUE;

			/* we found the nodaemoncheck option */
			else if (!strcmp(variables[x], "nodaemoncheck"))
				daemon_check = FALSE;
		}

		/* we found the string-to-expand argument */
		else if (!strcmp(variables[x], "expand")) {
			x++;
			if (variables[x] == NULL) {
				error = TRUE;
				break;
			}
			strncpy(to_expand, variables[x], MAX_COMMAND_BUFFER);
			to_expand[MAX_COMMAND_BUFFER-1] = '\0';
		}

		/* we found the CSV output option */
		else if (!strcmp(variables[x], "csvoutput")) {
			display_header = FALSE;
			content_type = CSV_CONTENT;
		}

		/* we found the JSON output option */
		else if (!strcmp(variables[x], "jsonoutput")) {
			display_header = FALSE;
			content_type = JSON_CONTENT;
		}

		/* we received an invalid argument */
		else
			error = TRUE;

	}

	/* free memory allocated to the CGI variables */
	free_cgivars(variables);

	return error;
}

void display_hosts(void) {
	host *temp_host = NULL;
	hostsmember *temp_hostsmember = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char *processed_string = NULL;
	int options = 0;
	int odd = 0;
	char time_string[2][16];
	char *bg_class = "";
	int contact = 0;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"hosts\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sHost Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAlias/Description%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAddress%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAddress6%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sParent Hosts%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sMax. Check Attempts%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sRetry Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHost Check Command%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sObsess Over%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Active Checks%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Passive Checks%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Freshness%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFreshness Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDefault Contacts/Groups%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFirst Notification Delay%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEvent Handler%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Event Handler%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStalking Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Flap Detection%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLow Flap Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHigh Flap Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFlap Detection Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sProcess Performance Data%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Failure Prediction%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFailure Prediction Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%s2-D Coords%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%s3-D Coords%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStatusmap Image%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sVRML Image%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLogo Image%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sImage Alt%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sRetention Options%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Host%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");
		printf("<TR>\n");
		printf("<TH CLASS='data'>Host Name</TH>");
		printf("<TH CLASS='data'>Alias/Description</TH>");
		printf("<TH CLASS='data'>Address</TH>");
		printf("<TH CLASS='data'>Address6</TH>");
		printf("<TH CLASS='data'>Parent Hosts</TH>");
		printf("<TH CLASS='data'>Max. Check Attempts</TH>");
		printf("<TH CLASS='data'>Check Interval</TH>\n");
		printf("<TH CLASS='data'>Retry Interval</TH>\n");
		printf("<TH CLASS='data'>Host Check Command</TH>");
		printf("<TH CLASS='data'>Check Period</TH>");
		printf("<TH CLASS='data'>Obsess Over</TH>\n");
		printf("<TH CLASS='data'>Enable Active Checks</TH>\n");
		printf("<TH CLASS='data'>Enable Passive Checks</TH>\n");
		printf("<TH CLASS='data'>Check Freshness</TH>\n");
		printf("<TH CLASS='data'>Freshness Threshold</TH>\n");
		printf("<TH CLASS='data'>Default Contacts/Groups</TH>\n");
		printf("<TH CLASS='data'>Notification Interval</TH>");
		printf("<TH CLASS='data'>First Notification Delay</TH>");
		printf("<TH CLASS='data'>Notification Options</TH>");
		printf("<TH CLASS='data'>Notification Period</TH>");
		printf("<TH CLASS='data'>Event Handler</TH>");
		printf("<TH CLASS='data'>Enable Event Handler</TH>");
		printf("<TH CLASS='data'>Stalking Options</TH>\n");
		printf("<TH CLASS='data'>Enable Flap Detection</TH>");
		printf("<TH CLASS='data'>Low Flap Threshold</TH>");
		printf("<TH CLASS='data'>High Flap Threshold</TH>");
		printf("<TH CLASS='data'>Flap Detection Options</TH>\n");
		printf("<TH CLASS='data'>Process Performance Data</TH>");
		printf("<TH CLASS='data'>Enable Failure Prediction</TH>");
		printf("<TH CLASS='data'>Failure Prediction Options</TH>");
		printf("<TH CLASS='data'>Notes</TH>");
		printf("<TH CLASS='data'>Notes URL</TH>");
		printf("<TH CLASS='data'>Action URL</TH>");
		printf("<TH CLASS='data'>2-D Coords</TH>");
		printf("<TH CLASS='data'>3-D Coords</TH>");
		printf("<TH CLASS='data'>Statusmap Image</TH>");
		printf("<TH CLASS='data'>VRML Image</TH>");
		printf("<TH CLASS='data'>Logo Image</TH>");
		printf("<TH CLASS='data'>Image Alt</TH>");
		printf("<TH CLASS='data'>Retention Options</TH>");
		printf("</TR>\n");
	}

	/* check all the hosts... */
	for (temp_host = host_list; temp_host != NULL; temp_host = temp_host->next) if (((*to_expand) == '\0') || !strcmp(to_expand, temp_host->name)) {

			/* grab macros */
			grab_host_macros_r(mac, temp_host);

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;
				printf("{ \"host_name\": \"%s\", ", json_encode(temp_host->name));
				printf("\"alias\": \"%s\", ", json_encode(temp_host->alias));
				printf("\"address\": \"%s\", ", temp_host->address);
				printf("\"address6\": \"%s\", ", temp_host->address6);
				printf("\"parent_hosts\": [ ");

				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_host->name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_host->alias, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_host->address, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_host->address6, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				/* find a solution to print display_name if once set in host definition */
				/* printf("<TD CLASS='%s'><a name='%s'>%s</a></TD>\n",bg_class,url_encode(temp_host->name),(temp_host->display_name!=NULL)?temp_host->display_name:temp_host->name); */
				printf("<TD CLASS='%s'><a name='%s'><a href='%s?type=services&expand=%s'>%s</a></a></TD>\n", bg_class,
				       url_encode(temp_host->name), CONFIG_CGI, url_encode(temp_host->name), html_encode(temp_host->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_host->alias, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_host->address, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_host->address6, FALSE));
				printf("<TD CLASS='%s'>", bg_class);
			}

			for (temp_hostsmember = temp_host->parent_hosts; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

				if (temp_hostsmember != temp_host->parent_hosts)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"host_name\": \"%s\" } ", json_encode(temp_hostsmember->host_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_hostsmember->host_name);
				else
					printf("<a href='%s?type=hosts&expand=%s'>%s</a>", CONFIG_CGI, url_encode(temp_hostsmember->host_name), html_encode(temp_hostsmember->host_name, FALSE));
			}

			if (temp_host->parent_hosts == NULL)
				printf("%s", (content_type == CSV_CONTENT || content_type == JSON_CONTENT) ? "" : "&nbsp;");

			get_interval_time_string(temp_host->check_interval, time_string[0], sizeof(time_string[0]));
			get_interval_time_string(temp_host->retry_interval, time_string[1], sizeof(time_string[1]));

			if (content_type == JSON_CONTENT) {
				printf("], ");
				printf("\"max_check_attempts\": %d, ", temp_host->max_attempts);
				printf("\"check_interval\": \"%s\", ", time_string[0]);
				printf("\"retry_interval\": \"%s\", ", time_string[1]);

				if (temp_host->host_check_command == NULL)
					printf("\"host_check_command\":	 null, ");
				else
					printf("\"host_check_command\": \"%s\", ", json_encode(temp_host->host_check_command));

				if (temp_host->check_period == NULL)
					printf("\"check_period\":  null, ");
				else
					printf("\"check_period\": \"%s\", ", json_encode(temp_host->check_period));

				printf("\"obsess_over\":  %s, ", (temp_host->obsess_over_host == TRUE) ? "true" : "false");
				printf("\"enable_active_checks\":  %s, ", (temp_host->checks_enabled == TRUE) ? "true" : "false");
				printf("\"enable_passive_checks\":  %s, ", (temp_host->accept_passive_host_checks == TRUE) ? "true" : "false");
				printf("\"check_freshness\":  %s, ", (temp_host->check_freshness == TRUE) ? "true" : "false");
				if (temp_host->freshness_threshold == 0)
					printf("\"freshness_threshold\":  \"auto-determined value\", ");
				else
					printf("\"freshness_threshold\": %d, ", temp_host->freshness_threshold);

				printf("\"default_contacts_default_groups\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%d%s%s", csv_data_enclosure, temp_host->max_attempts, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, time_string[0], csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, time_string[1], csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->host_check_command == NULL) ? "" : temp_host->host_check_command, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->check_period == NULL) ? "" : temp_host->check_period, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->obsess_over_host == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->checks_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->accept_passive_host_checks == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->check_freshness == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
				if (temp_host->freshness_threshold == 0)
					printf("Auto-determined value");
				else
					printf("%d", temp_host->freshness_threshold);
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_host->max_attempts);
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[0]);
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[1]);

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->host_check_command == NULL)
					printf("&nbsp;");
				else
					printf("<a href='%s?type=command&host=%s&expand=%s'>%s</a>", CONFIG_CGI, temp_host->name, url_encode(temp_host->host_check_command), html_encode(temp_host->host_check_command, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->check_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_host->check_period), html_encode(temp_host->check_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->obsess_over_host == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->checks_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->accept_passive_host_checks == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->check_freshness == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->freshness_threshold == 0)
					printf("Auto-determined value");
				else
					printf("%d seconds", temp_host->freshness_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			/* find all the contacts for this host... */
			contact = 0;
			for (temp_contactsmember = temp_host->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contact_name\": \"%s\" } ", json_encode(temp_contactsmember->contact_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactsmember->contact_name);
				else
					printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}
			for (temp_contactgroupsmember = temp_host->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contactgroup_name\": \"%s\" } ", json_encode(temp_contactgroupsmember->group_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactgroupsmember->group_name);
				else
					printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
			if (contact == 0)
				printf("%s", (content_type == CSV_CONTENT || content_type == JSON_CONTENT) ? "" : "&nbsp;");

			get_interval_time_string(temp_host->notification_interval, time_string[0], sizeof(time_string[0]));
			get_interval_time_string(temp_host->first_notification_delay, time_string[1], sizeof(time_string[1]));

			if (content_type == JSON_CONTENT) {
				printf("], ");
				printf("\"notification_interval\": \"%s\", ", time_string[0]);
				printf("\"first_notification_delay\": \"%s\", ", time_string[1]);
				printf("\"notification_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, time_string[0], csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, time_string[1], csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->notification_interval == 0) ? "<i>No Re-notification</I>" : html_encode(time_string[0], FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[1]);
				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_host->notify_on_down == TRUE) {
				options = 1;
				printf("%sDown%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_host->notify_on_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_host->notify_on_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_host->notify_on_flapping == TRUE) {
				printf("%s%sFlapping%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_host->notify_on_downtime == TRUE) {
				printf("%s%sDowntime%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				if (temp_host->notification_period == NULL)
					printf("\"notification_period\": null, ");
				else
					printf("\"notification_period\": \"%s\", ", json_encode(temp_host->notification_period));

				if (temp_host->event_handler == NULL)
					printf("\"event_handler\": null, ");
				else
					printf("\"event_handler\": \"%s\", ", json_encode(temp_host->event_handler));

				printf("\"enable_event_handler\": %s, ", (temp_host->event_handler_enabled == TRUE) ? "true" : "false");
				printf("\"stalking_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->notification_period == NULL) ? "" : temp_host->notification_period, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->event_handler == NULL) ? "" : temp_host->event_handler, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->event_handler_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);

				if (temp_host->notification_period == NULL)
					printf("&nbsp;");
				else
					printf("<a href='%s?type=timeperiods&expand=%s'>%s</a>", CONFIG_CGI, url_encode(temp_host->notification_period), html_encode(temp_host->notification_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->event_handler == NULL)
					printf("&nbsp");
				else
					/* printf("<a href='%s?type=commands&expand=%s'>%s</a></TD>\n",CONFIG_CGI,url_encode(strtok(temp_host->event_handler,"!")),html_encode(temp_host->event_handler,FALSE)); */
					printf("<a href='%s?type=command&host=%s&expand=%s'>%s</a>", CONFIG_CGI, temp_host->name, url_encode(temp_host->event_handler), html_encode(temp_host->event_handler, FALSE));

				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->event_handler_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_host->stalk_on_up == TRUE) {
				options = 1;
				printf("%sUp%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_host->stalk_on_down == TRUE) {
				printf("%s%sDown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_host->stalk_on_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"enable_flap_detection\": %s, ", (temp_host->flap_detection_enabled == TRUE) ? "true" : "false");
				if (temp_host->low_flap_threshold == 0.0)
					printf("\"low_flap_threshold\": \"Program-wide value\", ");
				else
					printf("\"low_flap_threshold\": %3.1f%%, ", temp_host->low_flap_threshold);

				if (temp_host->high_flap_threshold == 0.0)
					printf("\"high_flap_threshold\": \"Program-wide value\", ");
				else
					printf("\"high_flap_threshold\": %3.1f%%, ", temp_host->high_flap_threshold);

				printf("\"flap_detection_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->flap_detection_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				if (temp_host->low_flap_threshold == 0.0)
					printf("%sProgram-wide value%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%3.1f%%%s%s", csv_data_enclosure, temp_host->low_flap_threshold, csv_data_enclosure, csv_delimiter);
				if (temp_host->high_flap_threshold == 0.0)
					printf("%sProgram-wide value%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%3.1f%%%s%s", csv_data_enclosure, temp_host->high_flap_threshold, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->flap_detection_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->low_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_host->low_flap_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_host->high_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_host->high_flap_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_host->flap_detection_on_up == TRUE) {
				options = 1;
				printf("%sUp%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_host->flap_detection_on_down == TRUE) {
				printf("%s%sDown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_host->flap_detection_on_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"process_performance_data\": %s, ", (temp_host->process_performance_data == TRUE) ? "true" : "false");
				printf("\"enable_failure_prediction\": %s, ", (temp_host->failure_prediction_enabled == TRUE) ? "true" : "false");
				if (temp_host->failure_prediction_options == NULL)
					printf("\"failure_prediction_options\": null, ");
				else
					printf("\"failure_prediction_options\": \"%s\", ", temp_host->failure_prediction_options);
				if (temp_host->notes == NULL)
					printf("\"notes\": null, ");
				else
					printf("\"notes\": \"%s\", ", json_encode(temp_host->notes));
				if (temp_host->notes_url == NULL)
					printf("\"notes_url\": null, ");
				else
					printf("\"notes_url\": \"%s\", ", json_encode(temp_host->notes_url));
				if (temp_host->action_url == NULL)
					printf("\"action_url\": null, ");
				else
					printf("\"action_url\": \"%s\", ", json_encode(temp_host->action_url));
				if (temp_host->have_2d_coords == FALSE)
					printf("\"2d_coords\": null, ");
				else
					printf("\"2d_coords\": \"%d,%d\", ", temp_host->x_2d, temp_host->y_2d);
				if (temp_host->have_3d_coords == FALSE)
					printf("\"3d_coords\": null, ");
				else
					printf("\"3d_coords\": \"%.2f,%.2f,%.2f\", ", temp_host->x_3d, temp_host->y_3d, temp_host->z_3d);
				if (temp_host->statusmap_image == NULL)
					printf("\"statusmap_image\": null, ");
				else
					printf("\"statusmap_image\": \"%s\", ", temp_host->statusmap_image);
				if (temp_host->vrml_image == NULL)
					printf("\"vrml_image\": null, ");
				else
					printf("\"vrml_image\": \"%s\", ", json_encode(temp_host->vrml_image));
				if (temp_host->icon_image == NULL)
					printf("\"icon_image\": null, ");
				else
					printf("\"icon_image\": \"%s\", ", json_encode(temp_host->icon_image));
				if (temp_host->icon_image_alt == NULL)
					printf("\"icon_image_alt\": null, ");
				else
					printf("\"icon_image_alt\": \"%s\", ", json_encode(temp_host->icon_image_alt));
				printf("\"retention_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->process_performance_data == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->failure_prediction_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->failure_prediction_options == NULL) ? "" : temp_host->failure_prediction_options, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->notes == NULL) ? "" : temp_host->notes, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->notes_url == NULL) ? "" : temp_host->notes_url, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->action_url == NULL) ? "" : temp_host->action_url, csv_data_enclosure, csv_delimiter);
				if (temp_host->have_2d_coords == FALSE)
					printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%d,%d%s%s", csv_data_enclosure, temp_host->x_2d, temp_host->y_2d, csv_data_enclosure, csv_delimiter);
				if (temp_host->have_3d_coords == FALSE)
					printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
				else
					printf("%s%.2f,%.2f,%.2f%s%s", csv_data_enclosure, temp_host->x_3d, temp_host->y_3d, temp_host->z_3d, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->statusmap_image == NULL) ? "" : temp_host->statusmap_image, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->vrml_image == NULL) ? "" : temp_host->vrml_image, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->icon_image == NULL) ? "" : temp_host->icon_image, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_host->icon_image_alt == NULL) ? "" : temp_host->icon_image_alt, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->process_performance_data == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->failure_prediction_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->failure_prediction_options == NULL) ? "&nbsp;" : html_encode(temp_host->failure_prediction_options, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->notes == NULL) ? "&nbsp;" : html_encode(temp_host->notes, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->notes_url == NULL) ? "&nbsp;" : html_encode(temp_host->notes_url, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->action_url == NULL) ? "&nbsp;" : html_encode(temp_host->action_url, FALSE));

				if (temp_host->have_2d_coords == FALSE)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else
					printf("<TD CLASS='%s'>%d,%d</TD>\n", bg_class, temp_host->x_2d, temp_host->y_2d);

				if (temp_host->have_3d_coords == FALSE)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else
					printf("<TD CLASS='%s'>%.2f,%.2f,%.2f</TD>\n", bg_class, temp_host->x_3d, temp_host->y_3d, temp_host->z_3d);

				if (temp_host->statusmap_image == NULL)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else
					printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>\n", bg_class, url_logo_images_path, temp_host->statusmap_image, html_encode(temp_host->statusmap_image, FALSE));

				if (temp_host->vrml_image == NULL)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else
					printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>\n", bg_class, url_logo_images_path, temp_host->vrml_image, html_encode(temp_host->vrml_image, FALSE));

				if (temp_host->icon_image == NULL)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else {
					process_macros_r(mac, temp_host->icon_image, &processed_string, 0);
					printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'>%s</TD>\n", bg_class, url_logo_images_path, processed_string, html_encode(temp_host->icon_image, FALSE));
					free(processed_string);
				}

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_host->icon_image_alt == NULL) ? "&nbsp;" : html_encode(temp_host->icon_image_alt, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_host->retain_status_information == TRUE) {
				options = 1;
				printf("%sStatus Information%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_host->retain_nonstatus_information == TRUE) {
				printf("%s%sNon-Status Information%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");

		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_hostgroups(void) {
	hostgroup *temp_hostgroup;
	hostsmember *temp_hostsmember;
	int odd = 0;
	int json_start = TRUE;
	char *bg_class = "";

	if (content_type == JSON_CONTENT) {
		printf("\"hostgroups\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sGroup Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDescription%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHost Members%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction URL%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Host Group%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");
		printf("<TR>\n");
		printf("<TH CLASS='data'>Group Name</TH>");
		printf("<TH CLASS='data'>Description</TH>");
		printf("<TH CLASS='data'>Host Members</TH>");
		printf("<TH CLASS='data'>Notes</TH>");
		printf("<TH CLASS='data'>Notes URL</TH>");
		printf("<TH CLASS='data'>Action URL</TH>");
		printf("</TR>\n");
	}

	/* check all the hostgroups... */
	for (temp_hostgroup = hostgroup_list; temp_hostgroup != NULL; temp_hostgroup = temp_hostgroup->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_hostgroup->group_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;
				printf("{ \"group_name\": \"%s\", ", json_encode(temp_hostgroup->group_name));
				printf("\"description\": \"%s\", ", json_encode(temp_hostgroup->alias));
				printf("\"hostgroup_members\": [");

				/* find all the services that are members of this servicegroup... */
				for (temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {
					if (temp_hostsmember != temp_hostgroup->members)
						printf(",");
					printf(" \"%s\"", json_encode(temp_hostsmember->host_name));
				}
				printf(" ], ");
				if (temp_hostgroup->notes == NULL)
					printf("\"notes\": null, ");
				else
					printf("\"notes\": \"%s\", ", json_encode(temp_hostgroup->notes));
				if (temp_hostgroup->notes_url == NULL)
					printf("\"notes_url\": null, ");
				else
					printf("\"notes_url\": \"%s\", ", json_encode(temp_hostgroup->notes_url));
				if (temp_hostgroup->action_url == NULL)
					printf("\"action_url\": null }");
				else
					printf("\"action_url\": \"%s\" }", json_encode(temp_hostgroup->action_url));
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_hostgroup->group_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_hostgroup->alias, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);

				/* find all the services that are members of this servicegroup... */
				for (temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {
					if (temp_hostsmember != temp_hostgroup->members)
						printf(", ");
					printf("%s", temp_hostsmember->host_name);
				}

				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_hostgroup->notes == NULL) ? "" : temp_hostgroup->notes, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_hostgroup->notes_url == NULL) ? "" : temp_hostgroup->notes_url, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s\n", csv_data_enclosure, (temp_hostgroup->action_url == NULL) ? "" : temp_hostgroup->action_url, csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_hostgroup->group_name, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_hostgroup->alias, FALSE));

				printf("<TD CLASS='%s'>", bg_class);

				/* find all the hosts that are members of this hostgroup... */
				for (temp_hostsmember = temp_hostgroup->members; temp_hostsmember != NULL; temp_hostsmember = temp_hostsmember->next) {

					if (temp_hostsmember != temp_hostgroup->members)
						printf(", ");
					printf("<A HREF='%s?type=hosts&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_hostsmember->host_name), html_encode(temp_hostsmember->host_name, FALSE));
				}
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_hostgroup->notes == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->notes, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_hostgroup->notes_url == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->notes_url, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_hostgroup->action_url == NULL) ? "&nbsp;" : html_encode(temp_hostgroup->action_url, FALSE));

				printf("</TR>\n");
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_servicegroups(void) {
	servicegroup *temp_servicegroup;
	servicesmember *temp_servicesmember;
	int odd = 0;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"servicegroups\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sGroup Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDescription%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sService Members%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction URL%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Service Group%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Group Name</TH>");
		printf("<TH CLASS='data'>Description</TH>");
		printf("<TH CLASS='data'>Service Members</TH>");
		printf("<TH CLASS='data'>Notes</TH>");
		printf("<TH CLASS='data'>Notes URL</TH>");
		printf("<TH CLASS='data'>Action URL</TH>");
		printf("</TR>\n");
	}

	/* check all the servicegroups... */
	for (temp_servicegroup = servicegroup_list; temp_servicegroup != NULL; temp_servicegroup = temp_servicegroup->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_servicegroup->group_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;
				printf("{ \"group_name\": \"%s\", ", json_encode(temp_servicegroup->group_name));
				printf("\"description\": \"%s\", ", json_encode(temp_servicegroup->alias));
				printf("\"servicegroup_members\": [");

				/* find all the services that are members of this servicegroup... */
				for (temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next) {
					if (temp_servicesmember != temp_servicegroup->members)
						printf(",");
					printf(" { \"host_name\": \"%s\", \"service_description\": \"%s\" }", json_encode(temp_servicesmember->host_name), json_encode(temp_servicesmember->service_description));
				}
				printf(" ], ");
				if (temp_servicegroup->notes == NULL)
					printf("\"notes\": null, ");
				else
					printf("\"notes\": \"%s\", ", json_encode(temp_servicegroup->notes));
				if (temp_servicegroup->notes_url == NULL)
					printf("\"notes_url\": null, ");
				else
					printf("\"notes_url\": \"%s\", ", json_encode(temp_servicegroup->notes_url));
				if (temp_servicegroup->action_url == NULL)
					printf("\"action_url\": null }");
				else
					printf("\"action_url\": \"%s\" }", json_encode(temp_servicegroup->action_url));
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_servicegroup->group_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_servicegroup->alias, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);

				/* find all the services that are members of this servicegroup... */
				for (temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next)
					printf("%s%s / %s", (temp_servicesmember == temp_servicegroup->members) ? "" : ", ", temp_servicesmember->host_name, temp_servicesmember->service_description);

				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_servicegroup->notes == NULL) ? "" : temp_servicegroup->notes, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_servicegroup->notes_url == NULL) ? "" : temp_servicegroup->notes_url, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s\n", csv_data_enclosure, (temp_servicegroup->action_url == NULL) ? "" : temp_servicegroup->action_url, csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_servicegroup->group_name, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_servicegroup->alias, FALSE));

				printf("<TD CLASS='%s'>", bg_class);

				/* find all the services that are members of this servicegroup... */
				for (temp_servicesmember = temp_servicegroup->members; temp_servicesmember != NULL; temp_servicesmember = temp_servicesmember->next) {

					printf("%s<A HREF='%s?type=hosts&expand=%s'>%s</A> / ", (temp_servicesmember == temp_servicegroup->members) ? "" : ", ", CONFIG_CGI, url_encode(temp_servicesmember->host_name), html_encode(temp_servicesmember->host_name, FALSE));

					printf("<A HREF='%s?type=services&expand=%s#%s;", CONFIG_CGI, url_encode(temp_servicesmember->host_name), url_encode(temp_servicesmember->host_name));
					printf("%s'>%s</A>", url_encode(temp_servicesmember->service_description), html_encode(temp_servicesmember->service_description, FALSE));
				}

				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_servicegroup->notes == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->notes, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_servicegroup->notes_url == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->notes_url, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_servicegroup->action_url == NULL) ? "&nbsp;" : html_encode(temp_servicegroup->action_url, FALSE));

				printf("</TR>\n");
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_contacts(void) {
	contact *temp_contact;
	commandsmember *temp_commandsmember;
	int odd = 0;
	int options;
	int found;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"contacts\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sContact Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAlias%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEmail Address%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sPager Address/Number%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sService Notification Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHost Notification Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sService Notification Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHost Notification Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sService Notification Commands%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHost Notification Commands%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sRetention Options%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Contact%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Contact Name</TH>");
		printf("<TH CLASS='data'>Alias</TH>");
		printf("<TH CLASS='data'>Email Address</TH>");
		printf("<TH CLASS='data'>Pager Address/Number</TH>");
		printf("<TH CLASS='data'>Service Notification Options</TH>");
		printf("<TH CLASS='data'>Host Notification Options</TH>");
		printf("<TH CLASS='data'>Service Notification Period</TH>");
		printf("<TH CLASS='data'>Host Notification Period</TH>");
		printf("<TH CLASS='data'>Service Notification Commands</TH>");
		printf("<TH CLASS='data'>Host Notification Commands</TH>");
		printf("<TH CLASS='data'>Retention Options</TH>");
		printf("</TR>\n");
	}

	/* check all contacts... */
	for (temp_contact = contact_list; temp_contact != NULL; temp_contact = temp_contact->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_contact->name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"contact_name\": \"%s\", ", json_encode(temp_contact->name));
				printf("\"alias\": \"%s\", ", json_encode(temp_contact->alias));
				if (temp_contact->email == NULL)
					printf("\"email_address\": null, ");
				else
					printf("\"email_address\": \"%s\", ", json_encode(temp_contact->email));
				if (temp_contact->pager == NULL)
					printf("\"pager_number\": null, ");
				else
					printf("\"pager_number\": \"%s\", ", json_encode(temp_contact->pager));
				printf("\"service_notification_options\": [");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_contact->name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_contact->alias, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_contact->email == NULL) ? "" : temp_contact->email, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_contact->pager == NULL) ? "" : temp_contact->pager, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'>%s</a></TD>\n", bg_class, url_encode(temp_contact->name), html_encode(temp_contact->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_contact->alias, FALSE));
				printf("<TD CLASS='%s'><A HREF='mailto:%s'>%s</A></TD>\n", bg_class, (temp_contact->email == NULL) ? "&nbsp;" : url_encode(temp_contact->email), (temp_contact->email == NULL) ? "&nbsp;" : html_encode(temp_contact->email, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_contact->pager == NULL) ? "&nbsp;" : html_encode(temp_contact->pager, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_contact->notify_on_service_unknown == TRUE) {
				options = 1;
				printf("%sUnknown%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_contact->notify_on_service_warning == TRUE) {
				printf("%s%sWarning%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_service_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_service_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_service_flapping == TRUE) {
				printf("%s%sFlapping%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_service_downtime == TRUE) {
				printf("%s%sDowntime%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"host_notification_options\": [");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_contact->notify_on_host_down == TRUE) {
				options = 1;
				printf("%sDown%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_contact->notify_on_host_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_host_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_host_flapping == TRUE) {
				printf("%s%sFlapping%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_contact->notify_on_host_downtime == TRUE) {
				printf("%s%sDowntime%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				if (temp_contact->service_notification_period == NULL)
					printf("\"service_notification_period\": null, ");
				else
					printf("\"service_notification_period\": \"%s\", ", json_encode(temp_contact->service_notification_period));
				if (temp_contact->host_notification_period == NULL)
					printf("\"host_notification_period\": null, ");
				else
					printf("\"host_notification_period\": \"%s\", ", json_encode(temp_contact->host_notification_period));
				printf("\"service_notification_commands\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_contact->service_notification_period == NULL) ? "" : temp_contact->service_notification_period, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_contact->host_notification_period == NULL) ? "" : temp_contact->host_notification_period, csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);

				if (temp_contact->service_notification_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contact->service_notification_period), html_encode(temp_contact->service_notification_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>\n", bg_class);
				if (temp_contact->host_notification_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contact->host_notification_period), html_encode(temp_contact->host_notification_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			found = FALSE;
			for (temp_commandsmember = temp_contact->service_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

				if (temp_commandsmember != temp_contact->service_notification_commands)
					printf(", ");

				if (content_type == JSON_CONTENT) {
					printf("\"%s\"", json_encode(temp_commandsmember->command));
				} else if (content_type == CSV_CONTENT) {
					printf("%s", temp_commandsmember->command);
				} else {
					/* printf("<A HREF='%s?type=commands&expand=%s'>%s</A>",CONFIG_CGI,url_encode(strtok(temp_commandsmember->command,"!")),html_encode(temp_commandsmember->command,FALSE)); */
					printf("<A HREF='%s?type=command&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_commandsmember->command), html_encode(temp_commandsmember->command, FALSE));
				}

				found = TRUE;
			}

			if (found == FALSE && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"host_notification_commands\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
			}

			found = FALSE;
			for (temp_commandsmember = temp_contact->host_notification_commands; temp_commandsmember != NULL; temp_commandsmember = temp_commandsmember->next) {

				if (temp_commandsmember != temp_contact->host_notification_commands)
					printf(", ");

				if (content_type == JSON_CONTENT) {
					printf("\"%s\"", json_encode(temp_commandsmember->command));
				} else if (content_type == CSV_CONTENT) {
					printf("%s", temp_commandsmember->command);
				} else {
					/* printf("<A HREF='%s?type=commands&expand=%s'>%s</A>",CONFIG_CGI,url_encode(strtok(temp_commandsmember->command,"!")),html_encode(temp_commandsmember->command,FALSE)); */
					printf("<A HREF='%s?type=command&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_commandsmember->command), html_encode(temp_commandsmember->command, FALSE));
				}

				found = TRUE;
			}
			if (found == FALSE && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"retention_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_contact->retain_status_information == TRUE) {
				options = 1;
				printf("%sStatus Information%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_contact->retain_nonstatus_information == TRUE) {
				printf("%s%sNon-Status Information%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_contactgroups(void) {
	contactgroup *temp_contactgroup;
	contactsmember *temp_contactsmember;
	int odd = 0;
	char *bg_class;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"contactgroups\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sGroup Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDescription%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sContact Members%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Contact Group%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CELLSPACING=3 CELLPADDING=0>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Group Name</TH>\n");
		printf("<TH CLASS='data'>Description</TH>\n");
		printf("<TH CLASS='data'>Contact Members</TH>\n");
		printf("</TR>\n");
	}

	/* check all the contact groups... */
	for (temp_contactgroup = contactgroup_list; temp_contactgroup != NULL; temp_contactgroup = temp_contactgroup->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_contactgroup->group_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"contactgroup_name\": \"%s\", ", json_encode(temp_contactgroup->group_name));
				printf("\"alias\": \"%s\", ", json_encode(temp_contactgroup->alias));
				printf("\"contactgroup_members\": [");
				for (temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
					if (temp_contactsmember != temp_contactgroup->members)
						printf(",");

					printf(" \"%s\"", json_encode(temp_contactsmember->contact_name));
				}
				printf(" ] }");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_contactgroup->group_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_contactgroup->alias, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
				for (temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
					if (temp_contactsmember != temp_contactgroup->members)
						printf(", ");

					printf("%s", temp_contactsmember->contact_name);
				}
				printf("%s\n", csv_data_enclosure);
			} else {

				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_contactgroup->group_name), html_encode(temp_contactgroup->group_name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_contactgroup->alias, FALSE));

				/* find all the contact who are members of this contact group... */
				printf("<TD CLASS='%s'>", bg_class);
				for (temp_contactsmember = temp_contactgroup->members; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {

					if (temp_contactsmember != temp_contactgroup->members)
						printf(", ");

					printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
				}
				printf("</TD>\n");
				printf("</TR>\n");
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_services(void) {
	service *temp_service = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char *processed_string = NULL;
	char command_line[MAX_INPUT_BUFFER];
	int options;
	int odd = 0;
	char time_string[2][16];
	char *bg_class;
	int contact = 0;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"services\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sHost%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDescription%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sMax. Check Attempts%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNormal Check Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sRetry Check Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Command%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sParallelize%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sVolatile%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sObsess Over%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Active Checks%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Passive Checks%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCheck Freshness%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFreshness Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDefault Contacts/Groups%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Notifications%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFirst Notification Delay%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEvent Handler%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Event Handler%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sStalking Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Flap Detection%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLow Flap Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sHigh Flap Threshold%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFlap Detection Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sProcess Performance Data%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEnable Failure Prediction%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFailure Prediction Options%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotes URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAction URL%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLogo Image%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sImage Alt%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sRetention Options%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Service%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : "s Named or on Host "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Host</TH>\n");
		printf("<TH CLASS='data'>Description</TH>\n");
		printf("<TH CLASS='data'>Max. Check Attempts</TH>\n");
		printf("<TH CLASS='data'>Normal Check Interval</TH>\n");
		printf("<TH CLASS='data'>Retry Check Interval</TH>\n");
		printf("<TH CLASS='data'>Check Command</TH>\n");
		printf("<TH CLASS='data'>Check Period</TH>\n");
		printf("<TH CLASS='data'>Parallelize</TH>\n");
		printf("<TH CLASS='data'>Volatile</TH>\n");
		printf("<TH CLASS='data'>Obsess Over</TH>\n");
		printf("<TH CLASS='data'>Enable Active Checks</TH>\n");
		printf("<TH CLASS='data'>Enable Passive Checks</TH>\n");
		printf("<TH CLASS='data'>Check Freshness</TH>\n");
		printf("<TH CLASS='data'>Freshness Threshold</TH>\n");
		printf("<TH CLASS='data'>Default Contacts/Groups</TH>\n");
		printf("<TH CLASS='data'>Enable Notifications</TH>\n");
		printf("<TH CLASS='data'>Notification Interval</TH>\n");
		printf("<TH CLASS='data'>First Notification Delay</TH>\n");
		printf("<TH CLASS='data'>Notification Options</TH>\n");
		printf("<TH CLASS='data'>Notification Period</TH>\n");
		printf("<TH CLASS='data'>Event Handler</TH>");
		printf("<TH CLASS='data'>Enable Event Handler</TH>");
		printf("<TH CLASS='data'>Stalking Options</TH>\n");
		printf("<TH CLASS='data'>Enable Flap Detection</TH>");
		printf("<TH CLASS='data'>Low Flap Threshold</TH>");
		printf("<TH CLASS='data'>High Flap Threshold</TH>");
		printf("<TH CLASS='data'>Flap Detection Options</TH>");
		printf("<TH CLASS='data'>Process Performance Data</TH>");
		printf("<TH CLASS='data'>Enable Failure Prediction</TH>");
		printf("<TH CLASS='data'>Failure Prediction Options</TH>");
		printf("<TH CLASS='data'>Notes</TH>");
		printf("<TH CLASS='data'>Notes URL</TH>");
		printf("<TH CLASS='data'>Action URL</TH>");
		printf("<TH CLASS='data'>Logo Image</TH>");
		printf("<TH CLASS='data'>Image Alt</TH>");
		printf("<TH CLASS='data'>Retention Options</TH>");
		printf("</TR>\n");
	}

	/* check all the services... */
	for (temp_service = service_list; temp_service != NULL; temp_service = temp_service->next) {

		if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_service->host_name)) || (!strcmp(to_expand, temp_service->description))) {

			/* grab macros */
			grab_service_macros_r(mac, temp_service);

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			get_interval_time_string(temp_service->check_interval, time_string[0], sizeof(time_string[0]));
			get_interval_time_string(temp_service->retry_interval, time_string[1], sizeof(time_string[1]));
			strncpy(command_line, temp_service->service_check_command, sizeof(command_line));
			command_line[sizeof(command_line)-1] = '\x0';

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;
				printf("{ \"host_name\": \"%s\", ", json_encode(temp_service->host_name));
				printf("\"description\": \"%s\", ", json_encode(temp_service->description));
				printf("\"max_check_attempts\": %d, ", temp_service->max_attempts);
				printf("\"normal_check_interval\": \"%s\", ", time_string[0]);
				printf("\"retry_check_interval\": \"%s\", ", time_string[1]);
				printf("\"check_command\": \"%s\", ", json_encode(command_line));
				if (temp_service->check_period == NULL)
					printf("\"check_period\": null, ");
				else
					printf("\"check_period\": \"%s\", ", json_encode(temp_service->check_period));

				printf("\"parallelize\": %s, ", (temp_service->parallelize == TRUE) ? "true" : "false");
				printf("\"volatile\": %s, ", (temp_service->is_volatile == TRUE) ? "true" : "false");
				printf("\"obsess_over\": %s, ", (temp_service->obsess_over_service == TRUE) ? "true" : "false");
				printf("\"enable_active_checks\": %s, ", (temp_service->checks_enabled == TRUE) ? "true" : "false");
				printf("\"enable_passive_checks\": %s, ", (temp_service->accept_passive_service_checks == TRUE) ? "true" : "false");
				printf("\"check_freshness\": %s, ", (temp_service->check_freshness == TRUE) ? "true" : "false");
				if (temp_service->freshness_threshold == 0)
					printf("\"freshness_threshold\":  \"auto-determined value\", ");
				else
					printf("\"freshness_threshold\":  %d, ", temp_service->freshness_threshold);

				printf("\"default_contacts_default_groups\": [ ");

				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_service->host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_service->description, csv_data_enclosure, csv_delimiter);
				printf("%s%d%s%s", csv_data_enclosure, temp_service->max_attempts, csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, time_string[0], csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, time_string[1], csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, command_line, csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->check_period == NULL) ? "" : temp_service->check_period, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->parallelize == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->is_volatile == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->obsess_over_service == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->checks_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->accept_passive_service_checks == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->check_freshness == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
				if (temp_service->freshness_threshold == 0)
					printf("Auto-determined value");
				else
					printf("%d", temp_service->freshness_threshold);

				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);

			} else {

				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s;", bg_class, url_encode(temp_service->host_name));
				printf("%s'></A>", url_encode(temp_service->description));
				printf("<A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", CONFIG_CGI, url_encode(temp_service->host_name), html_encode(temp_service->host_name, FALSE));

				/* find a way to show display_name if set once */
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_service->description, FALSE));

				printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_service->max_attempts);

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[0]);

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[1]);

				printf("<TD CLASS='%s'><A HREF='%s?type=command&host=%s&service=%s&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, temp_service->host_name, temp_service->description, url_encode(command_line), html_encode(command_line, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->check_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_service->check_period), html_encode(temp_service->check_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->parallelize == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->is_volatile != FALSE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->obsess_over_service == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->checks_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->accept_passive_service_checks == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->check_freshness == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->freshness_threshold == 0)
					printf("Auto-determined value");
				else
					printf("%d seconds", temp_service->freshness_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			contact = 0;
			for (temp_contactsmember = temp_service->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contact_name\": \"%s\" } ", json_encode(temp_contactsmember->contact_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactsmember->contact_name);
				else
					printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}

			for (temp_contactgroupsmember = temp_service->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contactgroup_name\": \"%s\" } ", json_encode(temp_contactgroupsmember->group_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactgroupsmember->group_name);
				else
					printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
			if (contact == 0)
				printf("%s", (content_type == CSV_CONTENT || content_type == JSON_CONTENT) ? "" : "&nbsp;");

			get_interval_time_string(temp_service->notification_interval, time_string[0], sizeof(time_string[0]));
			get_interval_time_string(temp_service->first_notification_delay, time_string[1], sizeof(time_string[1]));

			if (content_type == JSON_CONTENT) {
				printf("], ");
				printf("\"enable_notifications\": %s, ", (temp_service->notifications_enabled == TRUE) ? "true" : "false");
				printf("\"notification_interval\": \"%s\", ", time_string[0]);
				printf("\"first_notification_delay\": \"%s\", ", time_string[1]);
				printf("\"notification_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->notifications_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->notification_interval == 0) ? "No Re-notification" : time_string[0], csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, time_string[1], csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->notifications_enabled == TRUE) ? "Yes" : "No");
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->notification_interval == 0) ? "<i>No Re-notification</i>" : html_encode(time_string[0], FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, time_string[1]);
				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_service->notify_on_unknown == TRUE) {
				options = 1;
				printf("%sUnknown%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_service->notify_on_warning == TRUE) {
				printf("%s%sWarning%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->notify_on_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->notify_on_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->notify_on_flapping == TRUE) {
				printf("%s%sFlapping%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->notify_on_downtime == TRUE) {
				printf("%s%sDowntime%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				if (temp_service->notification_period == NULL)
					printf("\"notification_period\": null, ");
				else
					printf("\"notification_period\": \"%s\", ", json_encode(temp_service->notification_period));

				if (temp_service->event_handler == NULL)
					printf("\"event_handler\": null, ");
				else
					printf("\"event_handler\": \"%s\", ", json_encode(temp_service->event_handler));

				printf("\"enable_event_handler\": %s, ", (temp_service->event_handler_enabled == TRUE) ? "true" : "false");
				printf("\"stalking_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->notification_period == NULL) ? "" : temp_service->notification_period, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->event_handler == NULL) ? "" : temp_service->event_handler, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->event_handler_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->notification_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_service->notification_period), html_encode(temp_service->notification_period, FALSE));
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->event_handler == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=command&host=%s&service=%s&expand=%s'>%s</A>", CONFIG_CGI, temp_service->host_name, temp_service->description, url_encode(temp_service->event_handler), html_encode(temp_service->event_handler, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->event_handler_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_service->stalk_on_ok == TRUE) {
				options = 1;
				printf("%sOk%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				printf("Ok");
			}
			if (temp_service->stalk_on_warning == TRUE) {
				printf("%s%sWarning%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->stalk_on_unknown == TRUE) {
				printf("%s%sUnknown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->stalk_on_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"enable_flap_detection\": %s, ", (temp_service->flap_detection_enabled == TRUE) ? "true" : "false");
				if (temp_service->low_flap_threshold == 0.0)
					printf("\"low_flap_threshold\": \"Program-wide value\", ");
				else
					printf("\"low_flap_threshold\": %3.1f%%, ", temp_service->low_flap_threshold);

				if (temp_service->high_flap_threshold == 0.0)
					printf("\"high_flap_threshold\": \"Program-wide value\", ");
				else
					printf("\"high_flap_threshold\": %3.1f%%, ", temp_service->high_flap_threshold);

				printf("\"flap_detection_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->flap_detection_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
				if (temp_service->low_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_service->low_flap_threshold);

				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
				if (temp_service->high_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_service->high_flap_threshold);

				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->flap_detection_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->low_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_service->low_flap_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_service->high_flap_threshold == 0.0)
					printf("Program-wide value");
				else
					printf("%3.1f%%", temp_service->high_flap_threshold);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_service->flap_detection_on_ok == TRUE) {
				options = 1;
				printf("%sOk%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_service->flap_detection_on_warning == TRUE) {
				printf("%s%sWarning%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->flap_detection_on_unknown == TRUE) {
				printf("%s%sUnknown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_service->flap_detection_on_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"process_performance_data\": %s, ", (temp_service->process_performance_data == TRUE) ? "true" : "false");
				printf("\"enable_failure_prediction\": %s, ", (temp_service->failure_prediction_enabled == TRUE) ? "true" : "false");
				if (temp_service->failure_prediction_options == NULL)
					printf("\"failure_prediction_options\": null, ");
				else
					printf("\"failure_prediction_options\": \"%s\", ", temp_service->failure_prediction_options);
				if (temp_service->notes == NULL)
					printf("\"notes\": null, ");
				else
					printf("\"notes\": \"%s\", ", json_encode(temp_service->notes));
				if (temp_service->notes_url == NULL)
					printf("\"notes_url\": null, ");
				else
					printf("\"notes_url\": \"%s\", ", json_encode(temp_service->notes_url));
				if (temp_service->action_url == NULL)
					printf("\"action_url\": null, ");
				else
					printf("\"action_url\": \"%s\", ", json_encode(temp_service->action_url));
				if (temp_service->icon_image == NULL)
					printf("\"icon_image\": null, ");
				else {
					process_macros(temp_service->icon_image, &processed_string, 0);
					printf("\"icon_image\": \"%s\", ", json_encode(processed_string));
					free(processed_string);
				}
				if (temp_service->icon_image_alt == NULL)
					printf("\"icon_image_alt\": null, ");
				else
					printf("\"icon_image_alt\": \"%s\", ", json_encode(temp_service->icon_image_alt));
				printf("\"retention_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->process_performance_data == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->failure_prediction_enabled == TRUE) ? "Yes" : "No", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->failure_prediction_options == NULL) ? "" : temp_service->failure_prediction_options, csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->notes == NULL) ? "" : temp_service->notes, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->notes_url == NULL) ? "" : temp_service->notes_url, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_service->action_url == NULL) ? "" : temp_service->action_url, csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
				if (temp_service->icon_image != NULL) {
					process_macros(temp_service->icon_image, &processed_string, 0);
					printf("%s", processed_string);
					free(processed_string);
				}
				printf("%s%s", csv_data_enclosure, csv_delimiter);

				printf("%s%s%s%s", csv_data_enclosure, (temp_service->icon_image_alt == NULL) ? "" : temp_service->icon_image_alt, csv_data_enclosure, csv_delimiter);

				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->process_performance_data == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->failure_prediction_enabled == TRUE) ? "Yes" : "No");

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->failure_prediction_options == NULL) ? "&nbsp;" : html_encode(temp_service->failure_prediction_options, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->notes == NULL) ? "&nbsp;" : html_encode(temp_service->notes, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->notes_url == NULL) ? "&nbsp;" : html_encode(temp_service->notes_url, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->action_url == NULL) ? "&nbsp;" : html_encode(temp_service->action_url, FALSE));

				if (temp_service->icon_image == NULL)
					printf("<TD CLASS='%s'>&nbsp;</TD>\n", bg_class);
				else {
					process_macros(temp_service->icon_image, &processed_string, 0);
					printf("<TD CLASS='%s' valign='center'><img src='%s%s' border='0' width='20' height='20'> %s</TD>\n", bg_class, url_logo_images_path, processed_string, html_encode(temp_service->icon_image, FALSE));
					free(processed_string);
				}

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_service->icon_image_alt == NULL) ? "&nbsp;" : html_encode(temp_service->icon_image_alt, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_service->retain_status_information == TRUE) {
				options = 1;
				printf("%sStatus Information%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
			}
			if (temp_service->retain_nonstatus_information == TRUE) {
				printf("%s%sNon-Status Information%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT) {
				printf(" ] }");
			} else if (content_type == CSV_CONTENT) {
				printf("%s\n", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("</TR>\n");
			}
		}
	}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_timeperiods(void) {
	timerange *temp_timerange = NULL;
	daterange *temp_daterange = NULL;
	timeperiod *temp_timeperiod = NULL;
	timeperiodexclusion *temp_timeperiodexclusion = NULL;
	char *months[12] = {"january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"};
	char *days[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
	int odd = 0;
	int day = 0;
	int x = 0;
	char *bg_class = "";
	char timestring[10];
	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	int line = 0;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"timeperiods\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sName%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sAlias/Description%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sExclusions%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDays/Dates%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sTimes%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Time Period%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Name</TH>\n");
		printf("<TH CLASS='data'>Alias/Description</TH>\n");
		printf("<TH CLASS='data'>Exclusions</TH>\n");
		printf("<TH CLASS='data'>Days/Dates</TH>\n");
		printf("<TH CLASS='data'>Times</TH>\n");
		printf("</TR>\n");
	}

	/* check all the time periods... */
	for (temp_timeperiod = timeperiod_list; temp_timeperiod != NULL; temp_timeperiod = temp_timeperiod->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_timeperiod->name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"timeperiod_name\": \"%s\", ", json_encode(temp_timeperiod->name));
				printf("\"alias\": \"%s\", ", json_encode(temp_timeperiod->alias));
				printf("\"exclusions\": [ ");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_timeperiod->name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_timeperiod->alias, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'>%s</A></TD>\n", bg_class, url_encode(temp_timeperiod->name), html_encode(temp_timeperiod->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_timeperiod->alias, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}

			for (temp_timeperiodexclusion = temp_timeperiod->exclusions; temp_timeperiodexclusion != NULL; temp_timeperiodexclusion = temp_timeperiodexclusion->next) {
				if (temp_timeperiodexclusion != temp_timeperiod->exclusions)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("\"%s\"", temp_timeperiodexclusion->timeperiod_name);
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_timeperiodexclusion->timeperiod_name);
				else
					printf("<A HREF='#%s'>%s</A>", url_encode(temp_timeperiodexclusion->timeperiod_name), html_encode(temp_timeperiodexclusion->timeperiod_name, FALSE));
			}

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"days_times\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>", bg_class);
			}

			line = 0;
			for (x = 0; x < DATERANGE_TYPES; x++) {
				for (temp_daterange = temp_timeperiod->exceptions[x]; temp_daterange != NULL; temp_daterange = temp_daterange->next) {

					line++;

					if (line > 1) {
						if (content_type == JSON_CONTENT) {
							printf(", { \"days_date\": \"");
						} else if (content_type == CSV_CONTENT) {
							printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
							printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
							printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
							printf("%s", csv_data_enclosure);
						} else
							printf("<TR><TD COLSPAN='3'></TD>\n<TD CLASS='%s'>", bg_class);
					} else {
						if (content_type == JSON_CONTENT)
							printf("{ \"days_date\": \"");
					}

					switch (temp_daterange->type) {
					case DATERANGE_CALENDAR_DATE:
						printf("%d-%02d-%02d", temp_daterange->syear, temp_daterange->smon + 1, temp_daterange->smday);
						if ((temp_daterange->smday != temp_daterange->emday) || (temp_daterange->smon != temp_daterange->emon) || (temp_daterange->syear != temp_daterange->eyear))
							printf(" - %d-%02d-%02d", temp_daterange->eyear, temp_daterange->emon + 1, temp_daterange->emday);
						if (temp_daterange->skip_interval > 1)
							printf(" / %d", temp_daterange->skip_interval);
						break;
					case DATERANGE_MONTH_DATE:
						printf("%s %d", months[temp_daterange->smon], temp_daterange->smday);
						if ((temp_daterange->smon != temp_daterange->emon) || (temp_daterange->smday != temp_daterange->emday)) {
							printf(" - %s %d", months[temp_daterange->emon], temp_daterange->emday);
							if (temp_daterange->skip_interval > 1)
								printf(" / %d", temp_daterange->skip_interval);
						}
						break;
					case DATERANGE_MONTH_DAY:
						printf("day %d", temp_daterange->smday);
						if (temp_daterange->smday != temp_daterange->emday) {
							printf(" - %d", temp_daterange->emday);
							if (temp_daterange->skip_interval > 1)
								printf(" / %d", temp_daterange->skip_interval);
						}
						break;
					case DATERANGE_MONTH_WEEK_DAY:
						printf("%s %d %s", days[temp_daterange->swday], temp_daterange->swday_offset, months[temp_daterange->smon]);
						if ((temp_daterange->smon != temp_daterange->emon) || (temp_daterange->swday != temp_daterange->ewday) || (temp_daterange->swday_offset != temp_daterange->ewday_offset)) {
							printf(" - %s %d %s", days[temp_daterange->ewday], temp_daterange->ewday_offset, months[temp_daterange->emon]);
							if (temp_daterange->skip_interval > 1)
								printf(" / %d", temp_daterange->skip_interval);
						}
						break;
					case DATERANGE_WEEK_DAY:
						printf("%s %d", days[temp_daterange->swday], temp_daterange->swday_offset);
						if ((temp_daterange->swday != temp_daterange->ewday) || (temp_daterange->swday_offset != temp_daterange->ewday_offset)) {
							printf(" - %s %d", days[temp_daterange->ewday], temp_daterange->ewday_offset);
							if (temp_daterange->skip_interval > 1)
								printf(" / %d", temp_daterange->skip_interval);
						}
						break;
					default:
						break;
					}

					if (content_type == JSON_CONTENT)
						printf("\", \"time\": [ ");
					else if (content_type == CSV_CONTENT) {
						printf("%s%s", csv_data_enclosure, csv_delimiter);
						printf("%s", csv_data_enclosure);
					} else
						printf("</TD>\n<TD CLASS='%s'>", bg_class);

					for (temp_timerange = temp_daterange->times; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

						if (temp_timerange != temp_daterange->times)
							printf(", ");

						hours = temp_timerange->range_start / 3600;
						minutes = (temp_timerange->range_start - (hours * 3600)) / 60;
						seconds = temp_timerange->range_start - (hours * 3600) - (minutes * 60);
						snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
						timestring[sizeof(timestring)-1] = '\x0';
						printf("%s%s - ", (content_type == JSON_CONTENT) ? "\"" : "", timestring);

						hours = temp_timerange->range_end / 3600;
						minutes = (temp_timerange->range_end - (hours * 3600)) / 60;
						seconds = temp_timerange->range_end - (hours * 3600) - (minutes * 60);
						snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
						timestring[sizeof(timestring)-1] = '\x0';
						printf("%s%s", timestring, (content_type == JSON_CONTENT) ? "\"" : "");
					}

					if (content_type == JSON_CONTENT)
						printf(" ] }");
					else if (content_type == CSV_CONTENT)
						printf("%s\n", csv_data_enclosure);
					else {
						printf("</TD>\n");
						printf("</TR>\n");
					}
				}
			}
			for (day = 0; day < 7; day++) {

				if (temp_timeperiod->days[day] == NULL)
					continue;

				line++;

				if (line > 1) {
					if (content_type == JSON_CONTENT)
						printf(", { \"days_date\": \"%s\",  \"time\": [ ", days[day]);
					else if (content_type == CSV_CONTENT) {
						printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
						printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
						printf("%s%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
						printf("%s%s%s%s", csv_data_enclosure, days[day], csv_data_enclosure, csv_delimiter);
						printf("%s", csv_data_enclosure);
					} else
						printf("<TR><TD COLSPAN='3'></TD>\n<TD CLASS='%s'>", bg_class);
				} else {
					if (content_type == JSON_CONTENT)
						printf("{ \"days_date\": \"%s\",  \"time\": [ ", days[day]);
					else if (content_type == CSV_CONTENT) {
						printf("%s%s%s", days[day], csv_data_enclosure, csv_delimiter);
						printf("%s", csv_data_enclosure);
					} else
						printf("%s</TD>\n<TD CLASS='%s'>", days[day], bg_class);
				}


				for (temp_timerange = temp_timeperiod->days[day]; temp_timerange != NULL; temp_timerange = temp_timerange->next) {

					if (temp_timerange != temp_timeperiod->days[day])
						printf(", ");

					hours = temp_timerange->range_start / 3600;
					minutes = (temp_timerange->range_start - (hours * 3600)) / 60;
					seconds = temp_timerange->range_start - (hours * 3600) - (minutes * 60);
					snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
					timestring[sizeof(timestring)-1] = '\x0';
					printf("%s%s - ", (content_type == JSON_CONTENT) ? "\"" : "", timestring);

					hours = temp_timerange->range_end / 3600;
					minutes = (temp_timerange->range_end - (hours * 3600)) / 60;
					seconds = temp_timerange->range_end - (hours * 3600) - (minutes * 60);
					snprintf(timestring, sizeof(timestring) - 1, "%02d:%02d:%02d", hours, minutes, seconds);
					timestring[sizeof(timestring)-1] = '\x0';
					printf("%s%s", timestring, (content_type == JSON_CONTENT) ? "\"" : "");
				}

				if (content_type == JSON_CONTENT)
					printf(" ] }");
				else if (content_type == CSV_CONTENT)
					printf("%s\n", csv_data_enclosure);
				else {
					printf("</TD>\n");
					printf("</TR>\n");
				}
			}

			if (content_type == JSON_CONTENT)
				printf(" ] }\n");

			if (line == 0 && content_type != JSON_CONTENT) {

				if (content_type == CSV_CONTENT) {
					printf("%s%s", csv_data_enclosure, csv_delimiter);
					printf("%s%s\n", csv_data_enclosure, csv_data_enclosure);
				} else {
					printf("</TD>\n<TD></TD>\n");
					printf("</TR>\n");
				}
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_commands(void) {
	command *temp_command;
	int odd = 0;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"commands\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sCommand Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sCommand Line%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Command%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR><TH CLASS='data'>Command Name</TH><TH CLASS='data'>Command Line</TH></TR>\n");
	}

	/* check all commands */
	for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_command->name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataEven";
			} else {
				odd = 1;
				bg_class = "dataOdd";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"command_name\": \"%s\", ", json_encode(temp_command->name));
				printf("\"command_line\": \"%s\" }", json_encode(temp_command->command_line));
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_command->name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s\n", csv_data_enclosure, temp_command->command_line, csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_command->name), html_encode(temp_command->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_command->command_line, FALSE));

				printf("</TR>\n");
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_servicedependencies(void) {
	servicedependency *temp_sd;
	int odd = 0;
	int options;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"servicedependencies\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sDependent Host%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependent Service%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sMaster Host%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sMaster Service%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Type%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Failure Options%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Service Dependencie%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : "s Involving Host "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data' COLSPAN=2>Dependent Service</TH>");
		printf("<TH CLASS='data' COLSPAN=2>Master Service</TH>");
		printf("</TR>\n");
		printf("<TR>\n");
		printf("<TH CLASS='data'>Host</TH>");
		printf("<TH CLASS='data'>Service</TH>");
		printf("<TH CLASS='data'>Host</TH>");
		printf("<TH CLASS='data'>Service</TH>");
		printf("<TH CLASS='data'>Dependency Type</TH>");
		printf("<TH CLASS='data'>Dependency Period</TH>");
		printf("<TH CLASS='data'>Dependency Failure Options</TH>");
		printf("</TR>\n");
	}

	/* check all the service dependencies... */
	for (temp_sd = servicedependency_list; temp_sd != NULL; temp_sd = temp_sd->next)
		if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_sd->dependent_host_name)) || (!strcmp(to_expand, temp_sd->host_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"dependent_host_name\": \"%s\", ", json_encode(temp_sd->dependent_host_name));
				printf("\"dependent_service_description\": \"%s\", ", json_encode(temp_sd->dependent_service_description));
				printf("\"master_host_name\": \"%s\", ", json_encode(temp_sd->host_name));
				printf("\"master_service_description\": \"%s\", ", json_encode(temp_sd->service_description));
				printf("\"dependency_type\": \"%s\", ", (temp_sd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution");

				if (temp_sd->dependency_period == NULL)
					printf("\"dependency_period\": null, ");
				else
					printf("\"dependency_period\": \"%s\", ", json_encode(temp_sd->dependency_period));

				printf("\"dependency_failure_options\": [");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_sd->dependent_host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_sd->dependent_service_description, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_sd->host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_sd->service_description, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_sd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_sd->dependency_period == NULL) ? "" : temp_sd->dependency_period, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_sd->dependent_host_name), html_encode(temp_sd->dependent_host_name, FALSE));

				printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_sd->dependent_host_name), url_encode(temp_sd->dependent_host_name));
				printf("%s'>%s</A></TD>\n", url_encode(temp_sd->dependent_service_description), html_encode(temp_sd->dependent_service_description, FALSE));

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_sd->host_name), html_encode(temp_sd->host_name, FALSE));

				printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_sd->host_name), url_encode(temp_sd->host_name));
				printf("%s'>%s</A></TD>\n", url_encode(temp_sd->service_description), html_encode(temp_sd->service_description, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_sd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_sd->dependency_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_sd->dependency_period), html_encode(temp_sd->dependency_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_sd->fail_on_ok == TRUE) {
				printf("%sOk%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_sd->fail_on_warning == TRUE) {
				printf("%s%sWarning%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_sd->fail_on_unknown == TRUE) {
				printf("%s%sUnknown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_sd->fail_on_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_sd->fail_on_pending == TRUE) {
				printf("%s%sPending%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_serviceescalations(void) {
	serviceescalation *temp_se = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char time_string[16] = "";
	int options = FALSE;
	int odd = 0;
	char *bg_class = "";
	int contact = 0;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"serviceescalations\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sHost%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDescription%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sContacts/Groups%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFirst Notification%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLast Notification%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEscalation Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEscalation Options%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Service Escalation%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : "s on Host "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Host</TH>");
		printf("<TH CLASS='data'>Description</TH>");
		printf("<TH CLASS='data'>Contacts/Groups</TH>");
		printf("<TH CLASS='data'>First Notification</TH>");
		printf("<TH CLASS='data'>Last Notification</TH>");
		printf("<TH CLASS='data'>Notification Interval</TH>");
		printf("<TH CLASS='data'>Escalation Period</TH>");
		printf("<TH CLASS='data'>Escalation Options</TH>");
		printf("</TR>\n");
	}

	/* check all the service escalations... */
	for (temp_se = serviceescalation_list; temp_se != NULL; temp_se = temp_se->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_se->host_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"host_name\": \"%s\", ", json_encode(temp_se->host_name));
				printf("\"service_description\": \"%s\", ", json_encode(temp_se->description));
				printf("\"contacts_contactgroups\": [");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_se->host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_se->description, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_se->host_name), html_encode(temp_se->host_name, FALSE));

				printf("<TD CLASS='%s'><A HREF='%s?type=services&expand=%s#%s;", bg_class, CONFIG_CGI, url_encode(temp_se->host_name), url_encode(temp_se->host_name));
				printf("%s'>%s</A></TD>\n", url_encode(temp_se->description), html_encode(temp_se->description, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}

			contact = 0;
			for (temp_contactsmember = temp_se->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contact_name\": \"%s\" } ", json_encode(temp_contactsmember->contact_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactsmember->contact_name);
				else
					printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}
			for (temp_contactgroupsmember = temp_se->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");

				if (content_type == JSON_CONTENT)
					printf("{ \"contactgroup_name\": \"%s\" } ", json_encode(temp_contactgroupsmember->group_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactgroupsmember->group_name);
				else
					printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
			if (contact == 0)
				printf("%s", (content_type == CSV_CONTENT || content_type == JSON_CONTENT) ? "" : "&nbsp;");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"first_notification\": %d, ", temp_se->first_notification);
#ifdef USE_ST_BASED_ESCAL_RANGES
				printf("\"first_warning_notification\": %d, ", temp_se->first_warning_notification);
				printf("\"first_critical_notification\": %d, ", temp_se->first_critical_notification);
				printf("\"first_unknown_notification\": %d, ", temp_se->first_unknown_notification);
#endif
				if (temp_se->last_notification == 0)
					printf("\"last_notification\": null, ");
				else
					printf("\"last_notification\": %d, ", temp_se->last_notification);
#ifdef USE_ST_BASED_ESCAL_RANGES
				if (temp_se->last_warning_notification == 0)
					printf("\"last_warning_notification\": null, ");
				else
					printf("\"last_warning_notification\": %d, ", temp_se->last_warning_notification);
				if (temp_se->last_critical_notification == 0)
					printf("\"last_warning_notification\": null, ");
				else
					printf("\"last_warning_notification\": %d, ", temp_se->last_critical_notification);
				if (temp_se->last_unknown_notification == 0)
					printf("\"last_warning_notification\": null, ");
				else
					printf("\"last_warning_notification\": %d, ", temp_se->last_unknown_notification);
#endif
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);

#ifndef USE_ST_BASED_ESCAL_RANGES
				printf("%s%d%s%s", csv_data_enclosure, temp_se->first_notification, csv_data_enclosure, csv_delimiter);
#else
				printf("%s%d, %d, %d, %d%s%s", csv_data_enclosure, temp_se->first_notification, temp_se->first_warning_notification, temp_se->first_critical_notification, temp_se->first_unknown_notification, csv_data_enclosure, csv_delimiter);
#endif
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

#ifndef USE_ST_BASED_ESCAL_RANGES
				printf("<TD CLASS='%s'>%d</TD>\n", bg_class, temp_se->first_notification);
#else
				printf("<TD CLASS='%s'>%d, %d, %d, %d</TD>\n", bg_class, temp_se->first_notification, temp_se->first_warning_notification, temp_se->first_critical_notification, temp_se->first_unknown_notification);
#endif

				printf("<TD CLASS='%s'>", bg_class);
			}

			if (content_type != JSON_CONTENT) {
#ifdef USE_ST_BASED_ESCAL_RANGES
				if (temp_se->last_notification == 0)
					printf("Infinity, ");
				else
					printf("%d, ", temp_se->last_notification);
				if (temp_se->last_warning_notification == 0)
					printf("Infinity, ");
				else
					printf("%d, ", temp_se->last_warning_notification);
				if (temp_se->last_critical_notification == 0)
					printf("Infinity, ");
				else
					printf("%d, ", temp_se->last_critical_notification);
				if (temp_se->last_unknown_notification == 0)
					printf("Infinity");
				else
					printf("%d", temp_se->last_unknown_notification);
#else
				if (temp_se->last_notification == 0)
					printf("Infinity");
				else
					printf("%d", temp_se->last_notification);
#endif
			}

			get_interval_time_string(temp_se->notification_interval, time_string, sizeof(time_string));

			if (content_type == JSON_CONTENT) {
				printf("\"notification_interval\": \"%s\", ", (temp_se->notification_interval == 0.0) ? "Notify Only Once (No Re-notification)" : time_string);
				if (temp_se->escalation_period == NULL)
					printf("\"escalation_period\": null, ");
				else
					printf("\"escalation_period\": \"%s\", ", json_encode(temp_se->escalation_period));
				printf("\"escalation_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_se->notification_interval == 0.0) ? "Notify Only Once (No Re-notification)" : time_string, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_se->escalation_period == NULL) ? "" : temp_se->escalation_period, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_se->notification_interval == 0.0)
					printf("Notify Only Once (No Re-notification)");
				else
					printf("%s", time_string);
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_se->escalation_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_se->escalation_period), html_encode(temp_se->escalation_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_se->escalate_on_warning == TRUE) {
				printf("%sWarning%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_se->escalate_on_unknown == TRUE) {
				printf("%s%sUnknown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_se->escalate_on_critical == TRUE) {
				printf("%s%sCritical%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_se->escalate_on_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_hostdependencies(void) {
	hostdependency *temp_hd;
	int odd = 0;
	int options;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"hostdependencies\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sDependent Host%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sMaster Host%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Type%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sDependency Failure Options%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Host Dependencie%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : "s Involving Host "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Dependent Host</TH>");
		printf("<TH CLASS='data'>Master Host</TH>");
		printf("<TH CLASS='data'>Dependency Type</TH>");
		printf("<TH CLASS='data'>Dependency Period</TH>");
		printf("<TH CLASS='data'>Dependency Failure Options</TH>");
		printf("</TR>\n");
	}

	/* check all the host dependencies... */
	for (temp_hd = hostdependency_list; temp_hd != NULL; temp_hd = temp_hd->next)
		if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_hd->dependent_host_name)) || (!strcmp(to_expand, temp_hd->host_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"dependent_host_name\": \"%s\", ", json_encode(temp_hd->dependent_host_name));
				printf("\"master_host_name\": \"%s\", ", json_encode(temp_hd->host_name));
				printf("\"dependency_type\": \"%s\", ", (temp_hd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution");

				if (temp_hd->dependency_period == NULL)
					printf("\"dependency_period\": null, ");
				else
					printf("\"dependency_period\": \"%s\", ", json_encode(temp_hd->dependency_period));

				printf("\"dependency_failure_options\": [");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_hd->dependent_host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_hd->host_name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_hd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_hd->dependency_period == NULL) ? "" : temp_hd->dependency_period, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_hd->dependent_host_name), html_encode(temp_hd->dependent_host_name, FALSE));

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_hd->host_name), html_encode(temp_hd->host_name, FALSE));

				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_hd->dependency_type == NOTIFICATION_DEPENDENCY) ? "Notification" : "Check Execution");

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_hd->dependency_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_hd->dependency_period), html_encode(temp_hd->dependency_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_hd->fail_on_up == TRUE) {
				printf("%sUp%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_hd->fail_on_down == TRUE) {
				printf("%s%sDown%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_hd->fail_on_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_hd->fail_on_pending == TRUE) {
				printf("%s%sPending%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}

			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}

void display_hostescalations(void) {
	hostescalation *temp_he = NULL;
	contactsmember *temp_contactsmember = NULL;
	contactgroupsmember *temp_contactgroupsmember = NULL;
	char time_string[16] = "";
	int options = FALSE;
	int odd = 0;
	char *bg_class = "";
	int contact = 0;
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"hostescalations\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sHost%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sContacts/Groups%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sFirst Notification%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sLast Notification%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sNotification Interval%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEscalation Period%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sEscalation Options%s\n", csv_data_enclosure, csv_data_enclosure);
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Host Escalation%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : "s for Host "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR>\n");
		printf("<TH CLASS='data'>Host</TH>");
		printf("<TH CLASS='data'>Contacts/Groups</TH>");
		printf("<TH CLASS='data'>First Notification</TH>");
		printf("<TH CLASS='data'>Last Notification</TH>");
		printf("<TH CLASS='data'>Notification Interval</TH>");
		printf("<TH CLASS='data'>Escalation Period</TH>");
		printf("<TH CLASS='data'>Escalation Options</TH>");
		printf("</TR>\n");
	}

	/* check all the host escalations... */
	for (temp_he = hostescalation_list; temp_he != NULL; temp_he = temp_he->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_he->host_name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataOdd";
			} else {
				odd = 1;
				bg_class = "dataEven";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"host_name\": \"%s\", ", json_encode(temp_he->host_name));
				printf("\"contacts_contactgroups\": [");
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_he->host_name, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A HREF='%s?type=hosts&expand=%s'>%s</A></TD>\n", bg_class, CONFIG_CGI, url_encode(temp_he->host_name), html_encode(temp_he->host_name, FALSE));

				printf("<TD CLASS='%s'>", bg_class);
			}
			contact = 0;
			for (temp_contactsmember = temp_he->contacts; temp_contactsmember != NULL; temp_contactsmember = temp_contactsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");
				if (content_type == JSON_CONTENT)
					printf("{ \"contact_name\": \"%s\" } ", json_encode(temp_contactsmember->contact_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactsmember->contact_name);
				else
					printf("<A HREF='%s?type=contacts&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactsmember->contact_name), html_encode(temp_contactsmember->contact_name, FALSE));
			}
			for (temp_contactgroupsmember = temp_he->contact_groups; temp_contactgroupsmember != NULL; temp_contactgroupsmember = temp_contactgroupsmember->next) {
				contact++;
				if (contact > 1)
					printf(", ");
				if (content_type == JSON_CONTENT)
					printf("{ \"contactgroup_name\": \"%s\" } ", json_encode(temp_contactgroupsmember->group_name));
				else if (content_type == CSV_CONTENT)
					printf("%s", temp_contactgroupsmember->group_name);
				else
					printf("<A HREF='%s?type=contactgroups&expand=%s'>%s</A>\n", CONFIG_CGI, url_encode(temp_contactgroupsmember->group_name), html_encode(temp_contactgroupsmember->group_name, FALSE));
			}
			if (contact == 0)
				printf("%s", (content_type == CSV_CONTENT || content_type == JSON_CONTENT) ? "" : "&nbsp;");

			if (content_type == JSON_CONTENT) {
				printf(" ], ");
				printf("\"first_notification\": %d, ", temp_he->first_notification);
#ifdef USE_ST_BASED_ESCAL_RANGES
				printf("\"first_down_notification\": %d, ", temp_he->first_down_notification);
				printf("\"first_unreachable_notification\": %d, ", temp_he->first_unreachable_notification);
#endif
				if (temp_he->last_notification == 0)
					printf("\"last_notification\": null, ");
				else
					printf("\"last_notification\": %d, ", temp_he->last_notification);
#ifdef USE_ST_BASED_ESCAL_RANGES
				if (temp_he->last_down_notification == 0)
					printf("\"last_down_notification\": null, ");
				else
					printf("\"last_down_notification\": %d, ", temp_he->last_down_notification);
				if (temp_he->last_unreachable_notification == 0)
					printf("\"last_unreachable_notification\": null, ");
				else
					printf("\"last_unreachable_notification\": %d, ", temp_he->last_unreachable_notification);
#endif
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);

#ifndef USE_ST_BASED_ESCAL_RANGES
				printf("%s%d%s%s", csv_data_enclosure, temp_he->first_notification, csv_data_enclosure, csv_delimiter);
#else
				printf("%s%d, %d, %d%s%s", csv_data_enclosure, temp_he->first_notification, temp_he->first_down_notification, temp_he->first_unreachable_notification, csv_data_enclosure, csv_delimiter);
#endif
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");

#ifndef USE_ST_BASED_ESCAL_RANGES
				printf("<TD CLASS='%s'>%d</TD>", bg_class, temp_he->first_notification);
#else
				printf("<TD CLASS='%s'>%d, %d, %d</TD>", bg_class, temp_he->first_notification, temp_he->first_down_notification, temp_he->first_unreachable_notification);
#endif
				printf("<TD CLASS='%s'>", bg_class);
			}

			if (content_type != JSON_CONTENT) {
#ifdef USE_ST_BASED_ESCAL_RANGES
				if (temp_he->last_notification == 0)
					printf("Infinity, ");
				else
					printf("%d, ", temp_he->last_notification);
				if (temp_he->last_down_notification == 0)
					printf("Infinity, ");
				else
					printf("%d, ", temp_he->last_down_notification);
				if (temp_he->last_unreachable_notification == 0)
					printf("Infinity");
				else
					printf("%d", temp_he->last_unreachable_notification);
#else
				if (temp_he->last_notification == 0)
					printf("Infinity");
				else
					printf("%d", temp_he->last_notification);
#endif
			}

			get_interval_time_string(temp_he->notification_interval, time_string, sizeof(time_string));

			if (content_type == JSON_CONTENT) {
				printf("\"notification_interval\": \"%s\", ", (temp_he->notification_interval == 0.0) ? "Notify Only Once (No Re-notification)" : time_string);
				if (temp_he->escalation_period == NULL)
					printf("\"escalation_period\": null, ");
				else
					printf("\"escalation_period\": \"%s\", ", json_encode(temp_he->escalation_period));
				printf("\"escalation_options\": [ ");
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s", csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_he->notification_interval == 0.0) ? "Notify Only Once (No Re-notification)" : time_string, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, (temp_he->escalation_period == NULL) ? "" : temp_he->escalation_period, csv_data_enclosure, csv_delimiter);
				printf("%s", csv_data_enclosure);
			} else {
				printf("</TD>\n");
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, (temp_he->notification_interval == 0.0) ? "Notify Only Once (No Re-notification)" : time_string);

				printf("<TD CLASS='%s'>", bg_class);
				if (temp_he->escalation_period == NULL)
					printf("&nbsp;");
				else
					printf("<A HREF='%s?type=timeperiods&expand=%s'>%s</A>", CONFIG_CGI, url_encode(temp_he->escalation_period), html_encode(temp_he->escalation_period, FALSE));
				printf("</TD>\n");

				printf("<TD CLASS='%s'>", bg_class);
			}

			options = 0;
			if (temp_he->escalate_on_down == TRUE) {
				printf("%sDown%s", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_he->escalate_on_unreachable == TRUE) {
				printf("%s%sUnreachable%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (temp_he->escalate_on_recovery == TRUE) {
				printf("%s%sRecovery%s", (options) ? ", " : "", (content_type == JSON_CONTENT) ? "\"" : "", (content_type == JSON_CONTENT) ? "\"" : "");
				options = 1;
			}
			if (options == 0 && content_type != JSON_CONTENT)
				printf("None");

			if (content_type == JSON_CONTENT)
				printf(" ] }");
			else if (content_type == CSV_CONTENT)
				printf("%s\n", csv_data_enclosure);
			else
				printf("</TD>\n</TR>\n");
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}


void display_modules(void) {
	module *temp_module;
	int odd = 0;
	char *bg_class = "";
	int json_start = TRUE;

	if (content_type == JSON_CONTENT) {
		printf("\"modules\": [\n");
	} else if (content_type == CSV_CONTENT) {
		printf("%sModule Name%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sModule Type%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sModule Path%s%s", csv_data_enclosure, csv_data_enclosure, csv_delimiter);
		printf("%sModule Args%s", csv_data_enclosure, csv_data_enclosure);
		printf("\n");
	} else {
		printf("<DIV ALIGN=CENTER CLASS='dataTitle'>Module%s%s</DIV>\n",
		       (*to_expand == '\0' ? "s" : " "), (*to_expand == '\0' ? "" : escape_string(to_expand)));

		printf("<DIV ALIGN=CENTER>\n");

		printf("<TABLE BORDER=0 CLASS='data'>\n");

		printf("<TR><TH CLASS='data'>Module Name</TH><TH CLASS='data'>Module Type</TH><TH CLASS='data'>Module Path</TH><TH CLASS='data'>Module Args</TH></TR>\n");
	}

	/* check all modules */
	for (temp_module = module_list; temp_module != NULL; temp_module = temp_module->next) if (((*to_expand) == '\0') || (!strcmp(to_expand, temp_module->name))) {

			if (odd) {
				odd = 0;
				bg_class = "dataEven";
			} else {
				odd = 1;
				bg_class = "dataOdd";
			}

			/* print list in json format */
			if (content_type == JSON_CONTENT) {
				// always add a comma, except for the first line
				if (json_start == FALSE)
					printf(",\n");
				json_start = FALSE;

				printf("{ \"module_name\": \"%s\", ", json_encode(temp_module->name));
				printf("\"module_type\": \"%s\", ", json_encode(temp_module->type));
				printf("\"module_path\": \"%s\", ", json_encode(temp_module->path));
				printf("\"module_args\": \"%s\" }", json_encode(temp_module->args));
				/* print list in csv format */
			} else if (content_type == CSV_CONTENT) {
				printf("%s%s%s%s", csv_data_enclosure, temp_module->name, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_module->type, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s%s", csv_data_enclosure, temp_module->path, csv_data_enclosure, csv_delimiter);
				printf("%s%s%s\n", csv_data_enclosure, temp_module->args, csv_data_enclosure);
			} else {
				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_module->name), html_encode(temp_module->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_module->type, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_module->path, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_module->args, FALSE));

				printf("</TR>\n");
			}
		}

	if (content_type != CSV_CONTENT && content_type != JSON_CONTENT) {
		printf("</TABLE>\n");
		printf("</DIV>\n");
	} else if (content_type == JSON_CONTENT)
		printf("\n]\n");

	return;
}



char *hash_color(int i) {
	char c;

	/* This is actually optimized for MAX_COMMAND_ARGUMENTS==32 ... */

	if ((i % 32) < 16) {
		if ((i % 32) < 8) c = '7';
		else c = '4';
	} else {
		if ((i % 32) < 24) c = '6';
		else c = '5';
	}

	/* Computation for standard case */
	hashed_color[0] = '#';
	hashed_color[1] = hashed_color[2] = ((i % 2) ? c : '0');
	hashed_color[3] = hashed_color[4] = (((i / 2) % 2) ? c : '0');
	hashed_color[5] = hashed_color[6] = (((i / 4) % 2) ? c : '0');
	hashed_color[7] = '\0';

	/* Override shades of grey */
	if ((i % 8) == 7) hashed_color[1] = hashed_color[3] = '0';
	if ((i % 8) == 0) hashed_color[2] = hashed_color[3] = hashed_color[4] = hashed_color[6] = c;

	return(hashed_color);
}

void display_command_expansion(void) {
	command *temp_command;
	int odd = 0;
	char *bg_class = "";
	int i, j;
	char *c, *cc;
	char commandline[MAX_COMMAND_BUFFER];
	char *command_args[MAX_COMMAND_ARGUMENTS];
	int arg_count[MAX_COMMAND_ARGUMENTS];
	int lead_space[MAX_COMMAND_ARGUMENTS];
	int trail_space[MAX_COMMAND_ARGUMENTS];
	/* for host and service targets */
	host *hst = NULL;
	service *svc = NULL;
	char *processed_command;

	/* show host and/or service related raw command */
	hst = find_host(host_name);
	svc = find_service(host_name, service_desc);

	if (hst != NULL && svc == NULL)
		printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>Command Expansion for host '%s'</DIV></P>\n", host_name);
	else if (hst != NULL && svc != NULL)
		printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>Command Expansion for service '%s' on host '%s'</DIV></P>\n", service_desc, host_name);
	else
		printf("<P><DIV ALIGN=CENTER CLASS='dataTitle'>Command Expansion</DIV></P>\n");

	/* Parse to_expand into parts */
	for (i = 0; i < MAX_COMMAND_ARGUMENTS; i++) command_args[i] = NULL;
	for (i = 0, command_args[0] = cc = c = strdup(to_expand); c && ((*c) != '\0') && (i < MAX_COMMAND_ARGUMENTS); c++, cc++) {
		if ((*c) == '\\') c++;
		else if ((*c) == '!') {
			(*cc) = '\0';
			cc = c++;
			command_args[++i] = (c--);
		}
		(*cc) = (*c);
	}
	if ((*c) == '\0')(*cc) = '\0';
	/* Precompute indexes of dangling whitespace */
	for (i = 0; i < MAX_COMMAND_ARGUMENTS; i++) {
		for (cc = command_args[i], lead_space[i] = 0; cc && isspace(*cc); cc++, lead_space[i]++) ;
		trail_space[i] = 0;
		for (; cc && ((*cc) != '\0'); cc++) if (isspace(*cc)) trail_space[i]++;
			else trail_space[i] = 0;
	}

	printf("<P><DIV ALIGN=CENTER>\n");
	printf("<TABLE BORDER=0 CLASS='data'>\n");
	printf("<TR><TH CLASS='data'>Command Name</TH><TH CLASS='data'>Command Line</TH></TR>\n");

	if ((*to_expand) != '\0') {
		arg_count[0] = 0;

		printf("<TR CLASS='dataEven'><TD CLASS='dataEven'>To expand:</TD><TD CLASS='dataEven'>%s", escape_string(command_args[0]));
		for (i = 1; (i < MAX_COMMAND_ARGUMENTS) && command_args[i]; i++)
			printf("!<FONT\n   COLOR='%s'>%s</FONT>", hash_color(i), escape_string(command_args[i]));
		printf("\n</TD></TR>\n");

		/* check all commands */
		for (temp_command = command_list; temp_command != NULL; temp_command = temp_command->next) {

			if (!strcmp(temp_command->name, command_args[0])) {

				arg_count[0]++;

				if (odd) {
					odd = 0;
					bg_class = "dataEven";
				} else {
					odd = 1;
					bg_class = "dataOdd";
				}

				printf("<TR CLASS='%s'>\n", bg_class);

				printf("<TD CLASS='%s'><A NAME='%s'></A>%s</TD>\n", bg_class, url_encode(temp_command->name), html_encode(temp_command->name, FALSE));
				printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(temp_command->command_line, FALSE));

				printf("</TR>\n<TR CLASS='%s'>\n", bg_class);

				for (i = 1; i < MAX_COMMAND_ARGUMENTS; i++) arg_count[i] = 0;

				printf("<TD CLASS='%s' ALIGN='right'>-&gt;</TD>\n", bg_class);
				printf("<TD CLASS='%s'>", bg_class);
				strncpy(commandline, temp_command->command_line, MAX_COMMAND_BUFFER);
				commandline[MAX_COMMAND_BUFFER-1] = '\0';
				for (c = commandline; c && (cc = strstr(c, "$"));) {
					(*(cc++)) = '\0';
					printf("%s", html_encode(c, FALSE));
					if ((*cc) == '$') {
						/* Escaped '$' */
						printf("<FONT COLOR='#444444'>$</FONT>");
						c = (++cc);
					} else if (strncmp("ARG", cc, 3)) {
						/* Non-$ARGn$ macro */
						c = strstr(cc, "$");
						if (c)(*(c++)) = '\0';
						printf("<FONT COLOR='#777777'>$%s%s</FONT>", html_encode(cc, FALSE), (c ? "$" : ""));
						if (!c) printf("<FONT COLOR='#FF0000'> (not properly terminated)</FONT>");
					} else {
						/* $ARGn$ macro */
						for (c = (cc += 3); isdigit(*c); c++) ;
						if (((*c) == '\0') || ((*c) == '$')) {
							/* Index is numeric */
							i = atoi(cc);
							if ((i > 0) && (i <= MAX_COMMAND_ARGUMENTS)) {
								arg_count[i]++;
								if (command_args[i]) {
									if (*(command_args[i]) != '\0') printf("<FONT COLOR='%s'><B>%s%s%s</B></FONT>",
										                                       hash_color(i), ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""),
										                                       escape_string(command_args[i]), ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
									else printf("<FONT COLOR='#0000FF'>(empty)</FONT>");
								} else printf("<FONT COLOR='#0000FF'>(undefined)</FONT>");
							} else printf("<FONT COLOR='#FF0000'>(not a valid $ARGn$ index: %u)</FONT>", i);
							if ((*c) != '\0') c++;
							else printf("<FONT COLOR='#FF0000'> (not properly terminated)</FONT>");
						} else {
							/* Syntax err in index */
							c = strstr(cc, "$");
							printf("<FONT COLOR='#FF0000'>(not an $ARGn$ index: &quot;%s&quot;)</FONT>", html_encode(strtok(cc, "$"), FALSE));
							if (c) c++;
						}
					}
				}
				if (c) printf("%s", html_encode(c, FALSE));

				printf("</TD></TR>\n");

				for (i = 1; (i < MAX_COMMAND_ARGUMENTS) && (command_args[i]); i++) {
					if (arg_count[i] == 0) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'><FONT COLOR='#FF0000'>unused:</FONT></TD>\n", bg_class, bg_class);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='%s'>%s%s%s</FONT></TD></TR>\n", bg_class, i, hash_color(i),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""), escape_string(command_args[i]),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
					} else if (arg_count[i] > 1) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'>used %u x:</TD>\n", bg_class, bg_class, i);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='%s'>%s%s%s</FONT></TD></TR>\n", bg_class, i, hash_color(i),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "<U>&zwj;" : ""), escape_string(command_args[i]),
						       ((lead_space[i] > 0) || (trail_space[i] > 0) ? "&zwj;</U>" : ""));
					}
					if ((lead_space[i] > 0) || (trail_space[i] > 0)) {
						printf("<TR CLASS='%s'><TD CLASS='%s' ALIGN='right'><FONT COLOR='#0000FF'>dangling whitespace:</FONT></TD>\n", bg_class, bg_class);
						printf("<TD CLASS='%s'>$ARG%u$=<FONT COLOR='#0000FF'>", bg_class, i);
						for (c = command_args[i], j = 0; c && isspace(*c); c++, j++)
							if ((*c) == ' ')		printf("[SP]");
							else if ((*c) == '\f')	printf("[FF]");
							else if ((*c) == '\n')	printf("[LF]");
							else if ((*c) == '\r')	printf("[CR]");
							else if ((*c) == '\t')	printf("[HT]");
							else if ((*c) == '\v')	printf("[VT]");
							else			printf("[0x%x]", *c);
						printf("</FONT><FONT COLOR='%s'>", hash_color(i));
						for (; c && ((*c) != '\0') && (j < strlen(command_args[i]) - trail_space[i]); c++, j++) putchar(*c);
						printf("</FONT><FONT COLOR='#0000FF'>");
						for (; c && ((*c) != '\0'); c++)
							if ((*c) == ' ')		printf("[SP]");
							else if ((*c) == '\f')	printf("[FF]");
							else if ((*c) == '\n')	printf("[LF]");
							else if ((*c) == '\r')	printf("[CR]");
							else if ((*c) == '\t')	printf("[HT]");
							else if ((*c) == '\v')	printf("[VT]");
							else			printf("[0x%x]", *c);
						printf("</FONT></TD></TR>\n");
					}
				}

                                printf("<TR CLASS='%s'>\n", bg_class);

				/* host command */
				if (hst != NULL && svc == NULL) {
					grab_host_macros_r(mac,hst);
					process_macros_r(mac,temp_command->command_line, &processed_command, 0);
					printf("<TD CLASS='%s'>Raw commandline</TD>\n", bg_class);
                                	printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(processed_command, FALSE));
				}

				/* service command */
				if (hst != NULL && svc != NULL) {
					grab_host_macros_r(mac,hst);
					grab_service_macros_r(mac,svc);
					process_macros_r(mac, temp_command->command_line, &processed_command, 0);
					printf("<TD CLASS='%s'>Raw commandline</TD>\n", bg_class);
                                	printf("<TD CLASS='%s'>%s</TD>\n", bg_class, html_encode(processed_command, FALSE));
				}

                                printf("</TR>\n<TR CLASS='%s'>\n", bg_class);
			}

		}

		if (!arg_count[0]) {
			printf("<TR CLASS='dataOdd'><TD CLASS='dataOdd' ALIGN='right'><FONT\n");
			printf("COLOR='#FF0000'>Error:</FONT></TD><TD CLASS='dataOdd'><FONT COLOR='#FF0000'>No\n");
			printf("command &quot;%s&quot; found</FONT></TD></TR>\n", escape_string(command_args[0]));
		}
	}

	printf("<TR CLASS='dataEven'><TD CLASS='dataEven'>To expand:</TD><TD CLASS='dataEven'><FORM\n");
	printf("METHOD='GET' ACTION='%s'><INPUT TYPE='HIDDEN' NAME='type' VALUE='command'><INPUT\n", CONFIG_CGI);
	
	printf("TYPE='text' NAME='expand' SIZE='100%%' VALUE='%s'>\n", escape_string(to_expand));
	printf("<INPUT TYPE='SUBMIT' VALUE='Go'></FORM></TD></TR>\n");

	printf("</TABLE>\n");
	printf("</DIV></P>\n");

	return;
}

void display_options(void) {

	printf("<br><br>\n");

	printf("<div align=center class='reportSelectTitle'>Select Type of Config Data You Wish To View</div>\n");

	printf("<br><br>\n");

	printf("<form method=\"get\" action=\"%s\">\n", CONFIG_CGI);

	printf("<div align=center>\n");
	printf("<table border=0>\n");

	printf("<tr><td align=left class='reportSelectSubTitle'>Object Type:</td></tr>\n");
	printf("<tr><td align=left class='reportSelectItem'>");
	printf("<select name='type'>\n");
	printf("<option value='hosts' %s>Hosts\n", (display_type == DISPLAY_HOSTS) ? "SELECTED" : "");
	printf("<option value='hostdependencies' %s>Host Dependencies\n", (display_type == DISPLAY_HOSTDEPENDENCIES) ? "SELECTED" : "");
	printf("<option value='hostescalations' %s>Host Escalations\n", (display_type == DISPLAY_HOSTESCALATIONS) ? "SELECTED" : "");
	printf("<option value='hostgroups' %s>Host Groups\n", (display_type == DISPLAY_HOSTGROUPS) ? "SELECTED" : "");
	printf("<option value='services' %s>Services\n", (display_type == DISPLAY_SERVICES) ? "SELECTED" : "");
	printf("<option value='servicegroups' %s>Service Groups\n", (display_type == DISPLAY_SERVICEGROUPS) ? "SELECTED" : "");
	printf("<option value='servicedependencies' %s>Service Dependencies\n", (display_type == DISPLAY_SERVICEDEPENDENCIES) ? "SELECTED" : "");
	printf("<option value='serviceescalations' %s>Service Escalations\n", (display_type == DISPLAY_SERVICEESCALATIONS) ? "SELECTED" : "");
	printf("<option value='contacts' %s>Contacts\n", (display_type == DISPLAY_CONTACTS) ? "SELECTED" : "");
	printf("<option value='contactgroups' %s>Contact Groups\n", (display_type == DISPLAY_CONTACTGROUPS) ? "SELECTED" : "");
	printf("<option value='timeperiods' %s>Timeperiods\n", (display_type == DISPLAY_TIMEPERIODS) ? "SELECTED" : "");
	printf("<option value='modules' %s>Modules\n", (display_type == DISPLAY_MODULES) ? "SELECTED" : "");
	printf("<option value='commands' %s>Commands\n", (display_type == DISPLAY_COMMANDS) ? "SELECTED" : "");
	printf("<option value='command' %s>Command Expansion\n", (display_type == DISPLAY_COMMAND_EXPANSION) ? "SELECTED" : "");
	printf("</select>\n");
	printf("</td></tr>\n");

	printf("<tr><td class='reportSelectItem'><input type='submit' value='Continue'></td></tr>\n");
	printf("</table>\n");
	printf("</div>\n");

	printf("</form>\n");

	return;
}
