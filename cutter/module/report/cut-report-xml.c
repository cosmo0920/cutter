/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  g新部 Hiroyuki Ikezoe  <poincare@ikezoe.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 *  Boston, MA  02111-1307  USA
 *
 */

#ifdef HAVE_CONFIG_H
#  include <cutter/config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <cutter/cut-module-impl.h>
#include <cutter/cut-report.h>
#include <cutter/cut-runner.h>
#include <cutter/cut-test-result.h>
#include <cutter/cut-enum-types.h>

#define CUT_TYPE_REPORT_XML            cut_type_report_xml
#define CUT_REPORT_XML(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUT_TYPE_REPORT_XML, CutReportXML))
#define CUT_REPORT_XML_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CUT_TYPE_REPORT_XML, CutReportXMLClass))
#define CUT_IS_REPORT_XML(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUT_TYPE_REPORT_XML))
#define CUT_IS_REPORT_XML_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CUT_TYPE_REPORT_XML))
#define CUT_REPORT_XML_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CUT_TYPE_REPORT_XML, CutReportXMLClass))

typedef struct _CutReportXML CutReportXML;
typedef struct _CutReportXMLClass CutReportXMLClass;

struct _CutReportXML
{
    CutReport     object;
    CutRunner    *runner;
};

struct _CutReportXMLClass
{
    CutReportClass parent_class;
};

enum
{
    PROP_0,
    PROP_RUNNER
};

static GType cut_type_report_xml = 0;
static CutReportXMLClass *parent_class;

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void attach_to_runner             (CutListener *listener,
                                          CutRunner   *runner);
static void detach_from_runner           (CutListener *listener,
                                          CutRunner   *runner);

static gboolean result_to_file           (CutReport        *report,
                                          const gchar      *filename,
                                          CutReportFileMode mode);
static gchar   *get_result               (CutTestResult *result);
static gchar   *get_all_results          (CutReport   *report);
static gchar   *get_success_results      (CutReport   *report);
static gchar   *get_error_results        (CutReport   *report);
static gchar   *get_failure_results      (CutReport   *report);
static gchar   *get_pending_results      (CutReport   *report);
static gchar   *get_notification_results (CutReport   *report);
static gchar   *get_test_result          (CutReport   *report,
                                          const gchar *test_name);

static void
class_init (CutReportXMLClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;
    CutListenerClass *listener_class;
    CutReportClass *report_class;

    parent_class = g_type_class_peek_parent(klass);

    gobject_class = G_OBJECT_CLASS(klass);
    listener_class = CUT_LISTENER_CLASS(klass);
    report_class = CUT_REPORT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    listener_class->attach_to_runner   = attach_to_runner;
    listener_class->detach_from_runner = detach_from_runner;

    report_class->result_to_file           = result_to_file;
    report_class->get_all_results          = get_all_results;
    report_class->get_success_results      = get_success_results;
    report_class->get_error_results        = get_error_results;
    report_class->get_failure_results      = get_failure_results;
    report_class->get_pending_results      = get_pending_results;
    report_class->get_notification_results = get_notification_results;
    report_class->get_test_result          = get_test_result;

    spec = g_param_spec_object("cut-runner",
                               "CutRunner object",
                               "A CutRunner object",
                               CUT_TYPE_RUNNER,
                               G_PARAM_READWRITE);
    g_object_class_install_property(gobject_class, PROP_RUNNER, spec);
}

static void
init (CutReportXML *report)
{
    report->runner = NULL;
}

static void
register_type (GTypeModule *type_module)
{
    static const GTypeInfo info =
        {
            sizeof (CutReportXMLClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) class_init,
            NULL,           /* class_finalize */
            NULL,           /* class_data */
            sizeof(CutReportXML),
            0,
            (GInstanceInitFunc) init,
        };

    cut_type_report_xml =
        g_type_module_register_type(type_module,
                                    CUT_TYPE_REPORT,
                                    "CutReportXML",
                                    &info, 0);
}

G_MODULE_EXPORT GList *
CUT_MODULE_IMPL_INIT (GTypeModule *type_module)
{
    GList *registered_types = NULL;

    register_type(type_module);
    if (cut_type_report_xml)
        registered_types =
            g_list_prepend(registered_types,
                           (gchar *)g_type_name(cut_type_report_xml));

    return registered_types;
}

G_MODULE_EXPORT void
CUT_MODULE_IMPL_EXIT (void)
{
}

G_MODULE_EXPORT GObject *
CUT_MODULE_IMPL_INSTANTIATE (const gchar *first_property, va_list var_args)
{
    return g_object_new_valist(CUT_TYPE_REPORT_XML, first_property, var_args);
}

G_MODULE_EXPORT gchar *
CUT_MODULE_IMPL_GET_LOG_DOMAIN (void)
{
    return g_strdup(G_LOG_DOMAIN);
}

static void
dispose (GObject *object)
{
    CutReportXML *report = CUT_REPORT_XML(object);

    if (report->runner) {
        g_object_unref(report->runner);
        report->runner = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    CutReportXML *report = CUT_REPORT_XML(object);

    switch (prop_id) {
      case PROP_RUNNER:
        attach_to_runner(CUT_LISTENER(report), CUT_RUNNER(g_value_get_object(value)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    CutReportXML *report = CUT_REPORT_XML(object);

    switch (prop_id) {
      case PROP_RUNNER:
        g_value_set_object(value, G_OBJECT(report->runner));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
cb_start_test_suite (CutRunner *runner, CutTestSuite *test_suite,
                     CutReportXML *report)
{
}

static void
cb_start_test_case (CutRunner *runner, CutTestCase *test_case,
                    CutReportXML *report)
{
}

static void
cb_start_test (CutRunner *runner, CutTest *test, CutTestContext *test_context,
               CutReportXML *report)
{
}

static void
cb_success_test (CutRunner      *runner,
                 CutTest        *test,
                 CutTestContext *test_context,
                 CutTestResult  *result,
                 CutReportXML   *report)
{
}

static void
cb_failure_test (CutRunner      *runner,
                 CutTest        *test,
                 CutTestContext *test_context,
                 CutTestResult  *result,
                 CutReportXML   *report)
{
}

static void
cb_error_test (CutRunner      *runner,
               CutTest        *test,
               CutTestContext *test_context,
               CutTestResult  *result,
               CutReportXML   *report)
{
}

static void
cb_pending_test (CutRunner      *runner,
                 CutTest        *test,
                 CutTestContext *test_context,
                 CutTestResult  *result,
                 CutReportXML   *report)
{
}

static void
cb_notification_test (CutRunner      *runner,
                      CutTest        *test,
                      CutTestContext *test_context,
                      CutTestResult  *result,
                      CutReportXML   *report)
{
}

static void
cb_complete_test (CutRunner *runner, CutTest *test,
                  CutTestContext *test_context, CutReportXML *report)
{
}

static void
cb_complete_test_case (CutRunner *runner, CutTestCase *test_case,
                       CutReportXML *report)
{
}

static void
cb_complete_test_suite (CutRunner *runner, CutTestSuite *test_suite,
                        CutReportXML *report)
{
}

static void
cb_crashed (CutRunner *runner, const gchar *stack_trace,
            CutReportXML *report)
{
}

static void
connect_to_runner (CutReportXML *report, CutRunner *runner)
{
#define CONNECT(name) \
    g_signal_connect(runner, #name, G_CALLBACK(cb_ ## name), report)

    CONNECT(start_test_suite);
    CONNECT(start_test_case);
    CONNECT(start_test);

    CONNECT(success_test);
    CONNECT(failure_test);
    CONNECT(error_test);
    CONNECT(pending_test);
    CONNECT(notification_test);

    CONNECT(complete_test);
    CONNECT(complete_test_case);
    CONNECT(complete_test_suite);

    CONNECT(crashed);

#undef CONNECT
}

static void
disconnect_from_runner (CutReportXML *report, CutRunner *runner)
{
#define DISCONNECT(name)                                               \
    g_signal_handlers_disconnect_by_func(runner,                       \
                                         G_CALLBACK(cb_ ## name),      \
                                         report)

    DISCONNECT(start_test_suite);
    DISCONNECT(start_test_case);
    DISCONNECT(start_test);

    DISCONNECT(success_test);
    DISCONNECT(failure_test);
    DISCONNECT(error_test);
    DISCONNECT(pending_test);
    DISCONNECT(notification_test);

    DISCONNECT(complete_test);
    DISCONNECT(complete_test_case);
    DISCONNECT(complete_test_suite);

    DISCONNECT(crashed);

#undef DISCONNECT
}

static void
attach_to_runner (CutListener *listener,
                  CutRunner   *runner)
{
    CutReportXML *report = CUT_REPORT_XML(listener);
    if (report->runner)
        detach_from_runner(listener, report->runner);
    
    if (runner) {
        report->runner = g_object_ref(runner);
        connect_to_runner(CUT_REPORT_XML(listener), runner);
    }
}

static void
detach_from_runner (CutListener *listener,
                    CutRunner   *runner)
{
    CutReportXML *report = CUT_REPORT_XML(listener);
    if (report->runner != runner)
        return;

    disconnect_from_runner(report, runner);
    g_object_unref(report->runner);
    report->runner = NULL;
}

static gboolean
result_to_file (CutReport        *report,
                const gchar      *filename,
                CutReportFileMode mode)
{
    return FALSE;
}

static gchar *
get_all_results (CutReport *report)
{
    return NULL;
}

static const gchar *
result_status_to_name (CutTestResultStatus status)
{
    switch (status) {
      case CUT_TEST_RESULT_SUCCESS:
        return "success";
        break;
      case CUT_TEST_RESULT_FAILURE:
        return "failure";
        break;
      case CUT_TEST_RESULT_ERROR:
        return "error";
        break;
      case CUT_TEST_RESULT_PENDING:
        return "pending";
        break;
      case CUT_TEST_RESULT_NOTIFICATION:
        return "notification";
        break;
      default:
        return "unknown status";
        break;
    }
}

static void
append_indent (GString *string, guint indent)
{
    guint i;
    for (i = 0; i < indent; i++)
        g_string_append_c(string, ' ');

}

static void
append_element_with_value (GString *string, guint indent, const gchar *element_name, const gchar *value)
{
    gchar *escaped;
    append_indent(string, indent);
    escaped = g_markup_printf_escaped("<%s>%s</%s>\n", 
                                      element_name,
                                      value,
                                      element_name);
    g_string_append(string, escaped);
    g_free(escaped);
}

static void
append_element_valist (GString *string, guint indent, const gchar *element_name, va_list var_args)
{
  	const gchar *name;
  
  	name = element_name;
  
  	while (name) {
		const gchar *value = va_arg(var_args, gchar*);
        if (value)
            append_element_with_value(string, indent, name, value);
		name = va_arg(var_args, gchar*);
    }
}

static void
append_element_with_children (GString *string, guint indent, const gchar *element_name, 
                              const gchar *first_child_element, ...)
{
    gchar *escaped;
    va_list var_args;

    escaped = g_markup_escape_text(element_name, -1);
    append_indent(string, indent);
    g_string_append_printf(string, "<%s>\n", escaped);

    va_start(var_args, first_child_element);
    append_element_valist(string, indent + 2, first_child_element, var_args);
    va_end(var_args);

    append_indent(string, indent);
    g_string_append_printf(string, "</%s>\n", escaped);
    g_free(escaped);
}

static void
append_attribute (const gchar *key, const gchar *value, GString *string)
{
    guint indent = 6;

    if (!strcmp(key, "description"))
        return;

    append_indent(string, indent);
    g_string_append(string, "<option>\n");

    append_element_with_value(string, indent + 2, "name", key);
    append_element_with_value(string, indent + 2, "value", value);

    append_indent(string, indent);
    g_string_append(string, "</option>\n");
}

static void
append_test_info_to_string (GString *string, const gchar *element_name, CutTest *test)
{
    gchar *escaped;
    const gchar *description, *name;
    guint indent = 4;
    GHashTable *attributes;

    escaped = g_markup_escape_text(element_name, -1);
    append_indent(string, indent);
    g_string_append_printf(string, "<%s>\n", escaped);

    name = cut_test_get_name(test);
    if (name)
        append_element_with_value(string, indent + 2, "name", name);

    description = cut_test_get_description(test);
    if (description)
        append_element_with_value(string, indent + 2, "description", description);

    attributes = (GHashTable*)cut_test_get_attributes(test);
    if (attributes)
        g_hash_table_foreach(attributes, (GHFunc)append_attribute, string);

    append_indent(string, indent);
    g_string_append_printf(string, "</%s>\n", escaped);
    g_free(escaped);
}

static void
append_backtrace_to_string (GString *string, CutTestResult *result)
{
    gchar *line_string, *info_string;

    line_string = g_strdup_printf("%d", cut_test_result_get_line(result));
    info_string = g_strdup_printf("%s()", cut_test_result_get_function_name(result));

    append_element_with_children(string, 4, "backtrace",
                                 "file", cut_test_result_get_filename(result),
                                 "line", line_string,
                                 "info", info_string,
                                 NULL);
    g_free(line_string);
    g_free(info_string);
}

static void
append_test_result_to_string (GString *string, CutTestResult *result)
{
    CutTestResultStatus status;
    gchar *elapsed_string;
    const gchar *message;

    elapsed_string = g_strdup_printf("%f", cut_test_result_get_elapsed(result));
    message = cut_test_result_get_message(result);
    status = cut_test_result_get_status(result);

    append_element_with_value(string, 4, "status", result_status_to_name(status));
    if (message)
        append_element_with_value(string, 4, "detail", message);
    if (status != CUT_TEST_RESULT_SUCCESS)
        append_backtrace_to_string(string, result);
    append_element_with_value(string, 4, "elapsed", elapsed_string);
    g_free(elapsed_string);
}

static gchar *
get_result (CutTestResult *result)
{
    GString *xml = g_string_new("");

    g_string_append(xml, "  <result>\n");
    append_test_info_to_string(xml, "test-case", CUT_TEST(cut_test_result_get_test_case(result)));
    append_test_info_to_string(xml, "test", cut_test_result_get_test(result));
    append_test_result_to_string(xml, result);
    g_string_append(xml, "  </result>\n");

    return g_string_free(xml, FALSE);
}

static gchar *
get_status_results (CutReportXML *report, CutTestResultStatus status)
{
    const GList *node;
    GString *xml = g_string_new("");
    CutReportXML *report_xml = CUT_REPORT_XML(report);

    for (node = cut_runner_get_results(report_xml->runner);
         node;
         node = g_list_next(node)) {
        CutTestResult *result = node->data;
        gchar *result_string;

        if (status != cut_test_result_get_status(result))
            continue;

        result_string = get_result(result);
        g_string_append(xml, result_string);
        g_free(result_string);
    }

    return g_string_free(xml, FALSE);
}

static gchar *
get_success_results (CutReport *report)
{
    return get_status_results(CUT_REPORT_XML(report), CUT_TEST_RESULT_SUCCESS);
}

static gchar *
get_error_results (CutReport *report)
{
    return get_status_results(CUT_REPORT_XML(report), CUT_TEST_RESULT_ERROR);
}

static gchar *
get_failure_results (CutReport *report)
{
    return get_status_results(CUT_REPORT_XML(report), CUT_TEST_RESULT_FAILURE);
}

static gchar *
get_pending_results (CutReport *report)
{
    return get_status_results(CUT_REPORT_XML(report), CUT_TEST_RESULT_PENDING);
}

static gchar *
get_notification_results (CutReport *report)
{
    return get_status_results(CUT_REPORT_XML(report), CUT_TEST_RESULT_NOTIFICATION);
}

static gchar *
get_test_result (CutReport *report, const gchar *test_name)
{
    return NULL;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
