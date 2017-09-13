/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _XPL_CLIENT_H_
#define _XPL_CLIENT_H_

#include "plugin/x/ngs/include/ngs/client.h"
#include "plugin/x/ngs/include/ngs/interface/protocol_monitor_interface.h"

struct SHOW_VAR;

namespace xpl {
class Session;

class Client;

class Protocol_monitor : public ngs::Protocol_monitor_interface {
 public:
  Protocol_monitor() : m_client(0) {}
  void init(Client *client);

  void on_notice_warning_send() override;
  void on_notice_other_send() override;
  void on_error_send() override;
  void on_fatal_error_send() override;
  void on_init_error_send() override;
  void on_row_send() override;
  void on_send(long bytes_transferred) override;
  void on_receive(long bytes_transferred) override;
  void on_error_unknown_msg_type() override;

 private:
  Client *m_client;
};

class Client : public ngs::Client {
 public:
  Client(ngs::Connection_ptr connection, ngs::Server_interface &server,
         Client_id client_id, Protocol_monitor *pmon);
  virtual ~Client();

 public:  // impl ngs::Client_interface
  void on_session_close(ngs::Session_interface &s) override;
  void on_session_reset(ngs::Session_interface &s) override;

  void on_server_shutdown() override;
  void on_auth_timeout() override;

 public:  // impl ngs::Client
  void on_network_error(int error) override;
  std::string resolve_hostname() override;
  ngs::Capabilities_configurator *capabilities_configurator() override;

 public:
  bool is_handler_thd(THD *thd);

  void get_status_ssl_cipher_list(SHOW_VAR *var);

  void kill();
  ngs::shared_ptr<xpl::Session> get_session();

 private:
  bool is_localhost(const char *hostname);

  Protocol_monitor *m_protocol_monitor;
};

typedef ngs::shared_ptr<Client> Client_ptr;

}  // namespace xpl

#endif  // _XPL_CLIENT_H_
