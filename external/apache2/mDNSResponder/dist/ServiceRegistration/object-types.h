/* object-types.h
 *
 * Copyright (c) 2023 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Utility header to apply various behaviors to a list of objects.
 */

OBJECT_TYPE(address_query)
OBJECT_TYPE(adv_host)
OBJECT_TYPE(adv_instance)
OBJECT_TYPE(adv_instance_vec)
OBJECT_TYPE(adv_record)
OBJECT_TYPE(adv_record_vec)
OBJECT_TYPE(adv_update)
OBJECT_TYPE(advertising_proxy_conn_ref)
OBJECT_TYPE(comm)
OBJECT_TYPE(cti_connection)
OBJECT_TYPE(cti_prefix)
OBJECT_TYPE(cti_prefix_vec)
OBJECT_TYPE(cti_route)
OBJECT_TYPE(cti_route_vec)
OBJECT_TYPE(cti_service)
OBJECT_TYPE(cti_service_vec)
OBJECT_TYPE(dnssd_client)
OBJECT_TYPE(dnssd_query)
OBJECT_TYPE(dnssd_txn)
OBJECT_TYPE(dp_tracker)
OBJECT_TYPE(file_descriptor)
OBJECT_TYPE(interface)
OBJECT_TYPE(ifpermit_list)
OBJECT_TYPE(io)
OBJECT_TYPE(listener)
OBJECT_TYPE(message)
OBJECT_TYPE(nat64)
OBJECT_TYPE(nat64_prefix)
OBJECT_TYPE(nat64_ipv4_default_route_monitor)
OBJECT_TYPE(nat64_infra_prefix_monitor)
OBJECT_TYPE(nat64_thread_prefix_monitor)
OBJECT_TYPE(nat64_infra_prefix_publisher)
OBJECT_TYPE(nat64_br_prefix_publisher)
OBJECT_TYPE(node_type_tracker)
OBJECT_TYPE(nw_connection)
OBJECT_TYPE(nw_listener)
OBJECT_TYPE(nw_path_evaluator)
OBJECT_TYPE(omr_prefix)
OBJECT_TYPE(omr_publisher)
OBJECT_TYPE(omr_watcher)
OBJECT_TYPE(prefix_tracker)
OBJECT_TYPE(probe_state)
OBJECT_TYPE(question)
OBJECT_TYPE(route_tracker)
OBJECT_TYPE(rref)
OBJECT_TYPE(sdref)
OBJECT_TYPE(saref)
OBJECT_TYPE(service_publisher)
OBJECT_TYPE(service_tracker)
OBJECT_TYPE(srp_xpc_client)
OBJECT_TYPE(srpl_connection)
OBJECT_TYPE(srpl_domain)
OBJECT_TYPE(srpl_instance)
OBJECT_TYPE(srpl_instance_service)
OBJECT_TYPE(state_machine_event)
OBJECT_TYPE(subproc)
OBJECT_TYPE(thread_service)
OBJECT_TYPE(thread_tracker)
OBJECT_TYPE(wakeup)

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
