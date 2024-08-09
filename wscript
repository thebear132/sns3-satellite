# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os

def compile_generator(bld):
    base_path = 'contrib/satellite/model'
    data_path = '%s/../data/sgp4' % (base_path)
    if not os.path.isdir(data_path):
        return
    cxxflags = ' '.join(str(x) for x in bld.env['CXXFLAGS'])
    cxx = bld.env['COMPILER_CXX']
    src = '%s/constants-gen.cc' % data_path
    dst = '%s/cgen' % data_path
    header = '%s/iers-data.h' % (base_path)
    source = '%s/iers-data.cc' % (base_path)

    if not os.path.isfile(dst):
        #print('%s %s %s -o %s' % (cxx, src, cxxflags, dst))
        bld.exec_command('%s %s %s -o %s' % (cxx, cxxflags, src, dst))

    bld.exec_command('./%s header %s' % (dst, base_path))
    bld.exec_command('./%s source %s' % (dst, base_path))

def build(bld):
    module = bld.create_ns3_module('satellite', ['internet', 'propagation', 'antenna', 'csma', 'stats', 'traffic', 'flow-monitor', 'applications'])
    module.source = [
        'model/geo-coordinate.cc',
        'model/iers-data.cc',
        'model/julian-date.cc',
        'model/lora-adr-component.cc',
        'model/lora-beam-tag.cc',
        'model/lora-device-address-generator.cc',
        'model/lora-device-address.cc',
        'model/lora-end-device-status.cc',
        'model/lora-forwarder.cc',
        'model/lora-frame-header.cc',
        'model/lora-gateway-status.cc',
        'model/lora-logical-channel-helper.cc',
        'model/lora-logical-channel.cc',
        'model/lora-network-controller-components.cc',
        'model/lora-network-controller.cc',
        'model/lora-network-scheduler.cc',
        'model/lora-network-server.cc',
        'model/lora-network-status.cc',
        'model/lora-periodic-sender.cc',
        'model/lora-sub-band.cc',
        'model/lora-tag.cc',
        'model/lorawan-mac-command.cc',
        'model/lorawan-mac-end-device-class-a.cc',
        'model/lorawan-mac-end-device.cc',
        'model/lorawan-mac-gateway.cc',
        'model/lorawan-mac-header.cc',
        'model/lorawan-mac.cc',
        'model/satellite-address-tag.cc',
        'model/satellite-antenna-gain-pattern-container.cc',
        'model/satellite-antenna-gain-pattern.cc',
        'model/satellite-arp-cache.cc',
        'model/satellite-arq-buffer-context.cc',
        'model/satellite-arq-header.cc',
        'model/satellite-arq-sequence-number.cc',
        'model/satellite-base-encapsulator.cc',
        'model/satellite-base-fader-conf.cc',
        'model/satellite-base-fader.cc',
        'model/satellite-base-fading.cc',
        'model/satellite-base-trace-container.cc',
        'model/satellite-bbframe-conf.cc',
        'model/satellite-bbframe-container.cc',
        'model/satellite-bbframe.cc',
        'model/satellite-beam-channel-pair.cc',
        'model/satellite-beam-scheduler.cc',
        'model/satellite-bstp-controller.cc',
        'model/satellite-channel-estimation-error-container.cc',
        'model/satellite-channel-estimation-error.cc',
        'model/satellite-channel.cc',
        'model/satellite-cno-estimator.cc',
        'model/satellite-composite-sinr-output-trace-container.cc',
        'model/satellite-constant-interference.cc',
        'model/satellite-constant-position-mobility-model.cc',
        'model/satellite-control-message.cc',
        'model/satellite-crdsa-replica-tag.cc',
        'model/satellite-dama-entry.cc',
        'model/satellite-default-superframe-allocator.cc',
        'model/satellite-encap-pdu-status-tag.cc',
        'model/satellite-fading-external-input-trace-container.cc',
        'model/satellite-fading-external-input-trace.cc',
        'model/satellite-fading-input-trace-container.cc',
        'model/satellite-fading-input-trace.cc',
        'model/satellite-fading-oscillator.cc',
        'model/satellite-fading-output-trace-container.cc',
        'model/satellite-frame-allocator.cc',
        'model/satellite-frame-conf.cc',
        'model/satellite-free-space-loss.cc',
        'model/satellite-fwd-carrier-conf.cc',
        'model/satellite-fwd-link-scheduler-default.cc',
        'model/satellite-fwd-link-scheduler-time-slicing.cc',
        'model/satellite-fwd-link-scheduler.cc',
        'model/satellite-generic-stream-encapsulator-arq.cc',
        'model/satellite-generic-stream-encapsulator.cc',
        'model/satellite-geo-feeder-phy.cc',
        'model/satellite-geo-net-device.cc',
        'model/satellite-geo-user-phy.cc',
        'model/satellite-ground-station-address-tag.cc',
        'model/satellite-gse-header.cc',
        'model/satellite-gw-llc.cc',
        'model/satellite-gw-mac.cc',
        'model/satellite-gw-phy.cc',
        'model/satellite-handover-module.cc',
        'model/satellite-id-mapper.cc',
        'model/satellite-interference-elimination.cc',
        'model/satellite-interference-input-trace-container.cc',
        'model/satellite-interference-output-trace-container.cc',
        'model/satellite-isl-arbiter.cc',
        'model/satellite-isl-arbiter-unicast.cc',
        'model/satellite-interference.cc',
        'model/satellite-link-results.cc',
        'model/satellite-llc.cc',
        'model/satellite-log.cc',
        'model/satellite-loo-conf.cc',
        'model/satellite-loo-model.cc',
        'model/satellite-look-up-table.cc',
        'model/satellite-lora-phy-rx.cc',
        'model/satellite-lora-phy-tx.cc',
        'model/satellite-lorawan-net-device.cc',
        'model/satellite-lower-layer-service.cc',
        'model/satellite-mac-tag.cc',
        'model/satellite-mac.cc',
        'model/satellite-markov-conf.cc',
        'model/satellite-markov-container.cc',
        'model/satellite-markov-model.cc',
        'model/satellite-mobility-model.cc',
        'model/satellite-mobility-observer.cc',
        'model/satellite-mutual-information-table.cc',
        'model/satellite-ncc.cc',
        'model/satellite-net-device.cc',
        'model/satellite-node-info.cc',
        'model/satellite-on-off-application.cc',
        'model/satellite-orbiter-feeder-llc.cc',
        'model/satellite-orbiter-feeder-mac.cc',
        'model/satellite-orbiter-llc.cc',
        'model/satellite-orbiter-mac.cc',
        'model/satellite-orbiter-user-llc.cc',
        'model/satellite-orbiter-user-mac.cc',
        'model/satellite-packet-classifier.cc',
        'model/satellite-packet-trace.cc',
        'model/satellite-per-fragment-interference.cc',
        'model/satellite-per-packet-interference.cc',
        'model/satellite-perfect-interference-elimination.cc',
        'model/satellite-phy-rx-carrier-conf.cc',
        'model/satellite-phy-rx-carrier-marsala.cc',
        'model/satellite-phy-rx-carrier-per-frame.cc',
        'model/satellite-phy-rx-carrier-per-slot.cc',
        'model/satellite-phy-rx-carrier-per-window.cc',
        'model/satellite-phy-rx-carrier-uplink.cc',
        'model/satellite-phy-rx-carrier.cc',
        'model/satellite-phy-rx.cc',
        'model/satellite-phy-tx.cc',
        'model/satellite-phy.cc',
        'model/satellite-point-to-point-isl-channel.cc',
        'model/satellite-point-to-point-isl-net-device.cc',
        'model/satellite-position-allocator.cc',
        'model/satellite-position-input-trace-container.cc',
        'model/satellite-propagation-delay-model.cc',
        'model/satellite-queue.cc',
        'model/satellite-random-access-allocation-channel.cc',
        'model/satellite-random-access-container-conf.cc',
        'model/satellite-random-access-container.cc',
        'model/satellite-rayleigh-conf.cc',
        'model/satellite-rayleigh-model.cc',
        'model/satellite-request-manager.cc',
        'model/satellite-residual-interference-elimination.cc',
        'model/satellite-return-link-encapsulator-arq.cc',
        'model/satellite-return-link-encapsulator.cc',
        'model/satellite-rle-header.cc',
        'model/satellite-rtn-link-time.cc',
        'model/satellite-rx-cno-input-trace-container.cc',
        'model/satellite-rx-power-input-trace-container.cc',
        'model/satellite-rx-power-output-trace-container.cc',
        'model/satellite-scheduling-object.cc',
        'model/satellite-scpck-scheduler.cc',
        'model/satellite-sgp4-mobility-model.cc',
        'model/satellite-sgp4ext.cc',
        'model/satellite-sgp4io.cc',
        'model/satellite-sgp4unit.cc',
        'model/satellite-signal-parameters.cc',
        'model/satellite-simple-channel.cc',
        'model/satellite-simple-net-device.cc',
        'model/satellite-static-bstp.cc',
        'model/satellite-superframe-allocator.cc',
        'model/satellite-superframe-sequence.cc',
        'model/satellite-tbtp-container.cc',
        'model/satellite-time-tag.cc',
        'model/satellite-traced-interference.cc',
        'model/satellite-traced-mobility-model.cc',
        'model/satellite-uplink-info-tag.cc',
        'model/satellite-ut-llc.cc',
        'model/satellite-ut-mac-state.cc',
        'model/satellite-ut-mac.cc',
        'model/satellite-ut-phy.cc',
        'model/satellite-ut-scheduler.cc',
        'model/satellite-wave-form-conf.cc',
        'model/vector-extensions.cc',
        'utils/satellite-env-variables.cc',
        'utils/satellite-input-fstream-time-double-container.cc',
        'utils/satellite-input-fstream-time-long-double-container.cc',
        'utils/satellite-input-fstream-wrapper.cc',
        'utils/satellite-output-fstream-double-container.cc',
        'utils/satellite-output-fstream-long-double-container.cc',
        'utils/satellite-output-fstream-string-container.cc',
        'utils/satellite-output-fstream-wrapper.cc',
        'helper/lora-forwarder-helper.cc',
        'helper/lora-network-server-helper.cc',
        'helper/satellite-beam-helper.cc',
        'helper/satellite-beam-user-info.cc',
        'helper/satellite-cno-helper.cc',
        'helper/satellite-conf.cc',
        'helper/satellite-group-helper.cc',
        'helper/satellite-gw-helper.cc',
        'helper/satellite-gw-helper-dvb.cc',
        'helper/satellite-gw-helper-lora.cc',
        'helper/satellite-helper.cc',
        'helper/satellite-isl-arbiter-unicast-helper.cc',
        'helper/satellite-lora-conf.cc',
        'helper/satellite-on-off-helper.cc',
        'helper/satellite-orbiter-helper.cc',
        'helper/satellite-point-to-point-isl-helper.cc',
        'helper/satellite-traffic-helper.cc',
        'helper/satellite-user-helper.cc',
        'helper/satellite-ut-helper.cc',
        'helper/satellite-ut-helper-dvb.cc',
        'helper/satellite-ut-helper-lora.cc',
        'helper/simulation-helper.cc',
        'stats/satellite-frame-symbol-load-probe.cc',
        'stats/satellite-frame-user-load-probe.cc',
        'stats/satellite-phy-rx-carrier-packet-probe.cc',
        'stats/satellite-sinr-probe.cc',
        'stats/satellite-stats-antenna-gain-helper.cc',
        'stats/satellite-stats-backlogged-request-helper.cc',
        'stats/satellite-stats-beam-service-time-helper.cc',
        'stats/satellite-stats-capacity-request-helper.cc',
        'stats/satellite-stats-carrier-id-helper.cc',
        'stats/satellite-stats-composite-sinr-helper.cc',
        'stats/satellite-stats-delay-helper.cc',
        'stats/satellite-stats-frame-load-helper.cc',
        'stats/satellite-stats-frame-type-usage-helper.cc',
        'stats/satellite-stats-fwd-link-scheduler-symbol-rate-helper.cc',
        'stats/satellite-stats-helper-container.cc',
        'stats/satellite-stats-helper.cc',
        'stats/satellite-stats-jitter-helper.cc',
        'stats/satellite-stats-link-delay-helper.cc',
        'stats/satellite-stats-link-jitter-helper.cc',
        'stats/satellite-stats-link-modcod-helper.cc',
        'stats/satellite-stats-link-rx-power-helper.cc',
        'stats/satellite-stats-link-sinr-helper.cc',
        'stats/satellite-stats-marsala-correlation-helper.cc',
        'stats/satellite-stats-packet-collision-helper.cc',
        'stats/satellite-stats-packet-drop-rate-helper.cc',
        'stats/satellite-stats-packet-error-helper.cc',
        'stats/satellite-stats-plt-helper.cc',
        'stats/satellite-stats-queue-helper.cc',
        'stats/satellite-stats-rbdc-request-helper.cc',
        'stats/satellite-stats-resources-granted-helper.cc',
        'stats/satellite-stats-satellite-queue-helper.cc',
        'stats/satellite-stats-signalling-load-helper.cc',
        'stats/satellite-stats-throughput-helper.cc',
        'stats/satellite-stats-waveform-usage-helper.cc',
        'stats/satellite-stats-window-load-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('satellite')
    module_test.source = [
        'test/satellite-antenna-pattern-test.cc',
        'test/satellite-arq-seqno-test.cc',
        'test/satellite-arq-test.cc',
        'test/satellite-channel-estimation-error-test.cc',
        'test/satellite-cno-estimator-test.cc',
        'test/satellite-constellation-test.cc',
        'test/satellite-control-msg-container-test.cc',
        'test/satellite-cra-test.cc',
        'test/satellite-fading-external-input-trace-test.cc',
        'test/satellite-frame-allocator-test.cc',
        'test/satellite-fsl-test.cc',
        'test/satellite-geo-coordinate-test.cc',
        'test/satellite-gse-test.cc',
        'test/satellite-handover-test.cc',
        'test/satellite-interference-test.cc',
        'test/satellite-link-results-test.cc',
        'test/satellite-lora-test.cc',
        'test/satellite-mobility-observer-test.cc',
        'test/satellite-mobility-test.cc',
        'test/satellite-ncr-test.cc',
        'test/satellite-per-packet-if-test.cc',
        'test/satellite-performance-memory-test.cc',
        'test/satellite-periodic-control-message-test.cc',
        'test/satellite-random-access-test.cc',
        'test/satellite-regeneration-test.cc',
        'test/satellite-request-manager-test.cc',
        'test/satellite-rle-test.cc',
        'test/satellite-scenario-creation.cc',
        'test/satellite-simple-unicast.cc',
        'test/satellite-waveform-conf-test.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'satellite'
    headers.source = [
        'model/geo-coordinate.h',
        'model/iers-data.h',
        'model/julian-date.h',
        'model/lora-adr-component.h',
        'model/lora-beam-tag.h',
        'model/lora-device-address-generator.h',
        'model/lora-device-address.h',
        'model/lora-end-device-status.h',
        'model/lora-forwarder.h',
        'model/lora-frame-header.h',
        'model/lora-gateway-status.h',
        'model/lora-logical-channel-helper.h',
        'model/lora-logical-channel.h',
        'model/lora-network-controller-components.h',
        'model/lora-network-controller.h',
        'model/lora-network-scheduler.h',
        'model/lora-network-server.h',
        'model/lora-network-status.h',
        'model/lora-periodic-sender.h',
        'model/lora-sub-band.h',
        'model/lora-tag.h',
        'model/lorawan-mac-command.h',
        'model/lorawan-mac-end-device-class-a.h',
        'model/lorawan-mac-end-device.h',
        'model/lorawan-mac-gateway.h',
        'model/lorawan-mac-header.h',
        'model/lorawan-mac.h',
        'model/satellite-address-tag.h',
        'model/satellite-antenna-gain-pattern-container.h',
        'model/satellite-antenna-gain-pattern.h',
        'model/satellite-arp-cache.h',
        'model/satellite-arq-buffer-context.h',
        'model/satellite-arq-header.h',
        'model/satellite-arq-sequence-number.h',
        'model/satellite-base-encapsulator.h',
        'model/satellite-base-fader-conf.h',
        'model/satellite-base-fader.h',
        'model/satellite-base-fading.h',
        'model/satellite-base-trace-container.h',
        'model/satellite-bbframe-conf.h',
        'model/satellite-bbframe-container.h',
        'model/satellite-bbframe.h',
        'model/satellite-beam-channel-pair.h',
        'model/satellite-beam-scheduler.h',
        'model/satellite-bstp-controller.h',
        'model/satellite-channel-estimation-error-container.h',
        'model/satellite-channel-estimation-error.h',
        'model/satellite-channel.h',
        'model/satellite-cno-estimator.h',
        'model/satellite-composite-sinr-output-trace-container.h',
        'model/satellite-const-variables.h',
        'model/satellite-constant-interference.h',
        'model/satellite-constant-position-mobility-model.h',
        'model/satellite-control-message.h',
        'model/satellite-crdsa-replica-tag.h',
        'model/satellite-dama-entry.h',
        'model/satellite-default-superframe-allocator.h',
        'model/satellite-encap-pdu-status-tag.h',
        'model/satellite-enums.h',
        'model/satellite-fading-external-input-trace-container.h',
        'model/satellite-fading-external-input-trace.h',
        'model/satellite-fading-input-trace-container.h',
        'model/satellite-fading-input-trace.h',
        'model/satellite-fading-oscillator.h',
        'model/satellite-fading-output-trace-container.h',
        'model/satellite-frame-allocator.h',
        'model/satellite-frame-conf.h',
        'model/satellite-free-space-loss.h',
        'model/satellite-fwd-carrier-conf.h',
        'model/satellite-fwd-link-scheduler-default.h',
        'model/satellite-fwd-link-scheduler-time-slicing.h',
        'model/satellite-fwd-link-scheduler.h',
        'model/satellite-generic-stream-encapsulator-arq.h',
        'model/satellite-generic-stream-encapsulator.h',
        'model/satellite-geo-feeder-phy.h',
        'model/satellite-geo-net-device.h',
        'model/satellite-geo-user-phy.h',
        'model/satellite-ground-station-address-tag.h',
        'model/satellite-gse-header.h',
        'model/satellite-gw-llc.h',
        'model/satellite-gw-mac.h',
        'model/satellite-gw-phy.h',
        'model/satellite-handover-module.h',
        'model/satellite-id-mapper.h',
        'model/satellite-interference-elimination.h',
        'model/satellite-interference-input-trace-container.h',
        'model/satellite-interference-output-trace-container.h',
        'model/satellite-isl-arbiter.h',
        'model/satellite-isl-arbiter-unicast.h',
        'model/satellite-interference.h',
        'model/satellite-link-results.h',
        'model/satellite-llc.h',
        'model/satellite-log.h',
        'model/satellite-loo-conf.h',
        'model/satellite-loo-model.h',
        'model/satellite-look-up-table.h',
        'model/satellite-lora-phy-rx.h',
        'model/satellite-lora-phy-tx.h',
        'model/satellite-lorawan-net-device.h',
        'model/satellite-lower-layer-service.h',
        'model/satellite-mac-tag.h',
        'model/satellite-mac.h',
        'model/satellite-markov-conf.h',
        'model/satellite-markov-container.h',
        'model/satellite-markov-model.h',
        'model/satellite-mobility-model.h',
        'model/satellite-mobility-observer.h',
        'model/satellite-mutual-information-table.h',
        'model/satellite-ncc.h',
        'model/satellite-net-device.h',
        'model/satellite-node-info.h',
        'model/satellite-on-off-application.h',
        'model/satellite-orbiter-feeder-llc.h',
        'model/satellite-orbiter-feeder-mac.h',
        'model/satellite-orbiter-llc.h',
        'model/satellite-orbiter-mac.h',
        'model/satellite-orbiter-user-llc.h',
        'model/satellite-orbiter-user-mac.h',
        'model/satellite-packet-classifier.h',
        'model/satellite-packet-trace.h',
        'model/satellite-per-fragment-interference.h',
        'model/satellite-per-packet-interference.h',
        'model/satellite-perfect-interference-elimination.h',
        'model/satellite-phy-rx-carrier-conf.h',
        'model/satellite-phy-rx-carrier-marsala.h',
        'model/satellite-phy-rx-carrier-per-frame.h',
        'model/satellite-phy-rx-carrier-per-slot.h',
        'model/satellite-phy-rx-carrier-per-window.h',
        'model/satellite-phy-rx-carrier-uplink.h',
        'model/satellite-phy-rx-carrier.h',
        'model/satellite-phy-rx.h',
        'model/satellite-phy-tx.h',
        'model/satellite-phy.h',
        'model/satellite-point-to-point-isl-channel.h',
        'model/satellite-point-to-point-isl-net-device.h',
        'model/satellite-position-allocator.h',
        'model/satellite-position-input-trace-container.h',
        'model/satellite-propagation-delay-model.h',
        'model/satellite-queue.h',
        'model/satellite-random-access-allocation-channel.h',
        'model/satellite-random-access-container-conf.h',
        'model/satellite-random-access-container.h',
        'model/satellite-rayleigh-conf.h',
        'model/satellite-rayleigh-model.h',
        'model/satellite-request-manager.h',
        'model/satellite-residual-interference-elimination.h',
        'model/satellite-return-link-encapsulator-arq.h',
        'model/satellite-return-link-encapsulator.h',
        'model/satellite-rle-header.h',
        'model/satellite-rtn-link-time.h',
        'model/satellite-rx-cno-input-trace-container.h',
        'model/satellite-rx-power-input-trace-container.h',
        'model/satellite-rx-power-output-trace-container.h',
        'model/satellite-scheduling-object.h',
        'model/satellite-scpc-scheduler.h',
        'model/satellite-sgp4-mobility-model.h',
        'model/satellite-sgp4ext.h',
        'model/satellite-sgp4io.h',
        'model/satellite-sgp4unit.h',
        'model/satellite-signal-parameters.h',
        'model/satellite-simple-channel.h',
        'model/satellite-simple-net-device.h',
        'model/satellite-static-bstp.h',
        'model/satellite-superframe-allocator.h',
        'model/satellite-superframe-sequence.h',
        'model/satellite-tbtp-container.h',
        'model/satellite-time-tag.h',
        'model/satellite-traced-interference.h',
        'model/satellite-traced-mobility-model.h',
        'model/satellite-typedefs.h',
        'model/satellite-uplink-info-tag.h',
        'model/satellite-ut-llc.h',
        'model/satellite-ut-mac-state.h',
        'model/satellite-ut-mac.h',
        'model/satellite-ut-phy.h',
        'model/satellite-ut-scheduler.h',
        'model/satellite-utils.h',
        'model/satellite-wave-form-conf.h',
        'model/vector-extensions.h',
        'utils/satellite-env-variables.h',
        'utils/satellite-input-fstream-time-double-container.h',
        'utils/satellite-input-fstream-time-long-double-container.h',
        'utils/satellite-input-fstream-wrapper.h',
        'utils/satellite-output-fstream-double-container.h',
        'utils/satellite-output-fstream-long-double-container.h',
        'utils/satellite-output-fstream-string-container.h',
        'utils/satellite-output-fstream-wrapper.h',
        'helper/lora-forwarder-helper.h',
        'helper/lora-network-server-helper.h',
        'helper/satellite-beam-helper.h',
        'helper/satellite-beam-user-info.h',
        'helper/satellite-cno-helper.h',
        'helper/satellite-conf.h',
        'helper/satellite-group-helper.h',
        'helper/satellite-gw-helper.h',
        'helper/satellite-gw-helper-dvb.h',
        'helper/satellite-gw-helper-lora.h',
        'helper/satellite-helper.h',
        'helper/satellite-isl-arbiter-unicast-helper.h',
        'helper/satellite-lora-conf.h',
        'helper/satellite-on-off-helper.h',
        'helper/satellite-orbiter-helper.h',
        'helper/satellite-point-to-point-isl-helper.h',
        'helper/satellite-traffic-helper.h',
        'helper/satellite-user-helper.h',
        'helper/satellite-ut-helper.h',
        'helper/satellite-ut-helper-dvb.h',
        'helper/satellite-ut-helper-lora.h',
        'helper/simulation-helper.h',
        'stats/satellite-frame-symbol-load-probe.h',
        'stats/satellite-frame-user-load-probe.h',
        'stats/satellite-phy-rx-carrier-packet-probe.h',
        'stats/satellite-sinr-probe.h',
        'stats/satellite-stats-antenna-gain-helper.h',
        'stats/satellite-stats-backlogged-request-helper.h',
        'stats/satellite-stats-beam-service-time-helper.h',
        'stats/satellite-stats-capacity-request-helper.h',
        'stats/satellite-stats-carrier-id-helper.h',
        'stats/satellite-stats-composite-sinr-helper.h',
        'stats/satellite-stats-delay-helper.h',
        'stats/satellite-stats-frame-load-helper.h',
        'stats/satellite-stats-frame-type-usage-helper.h',
        'stats/satellite-stats-fwd-link-scheduler-symbol-rate-helper.h',
        'stats/satellite-stats-helper-container.h',
        'stats/satellite-stats-helper.h',
        'stats/satellite-stats-jitter-helper.h',
        'stats/satellite-stats-link-delay-helper.h',
        'stats/satellite-stats-link-jitter-helper.h',
        'stats/satellite-stats-link-modcod-helper.h',
        'stats/satellite-stats-link-rx-power-helper.h',
        'stats/satellite-stats-link-sinr-helper.h',
        'stats/satellite-stats-marsala-correlation-helper.h',
        'stats/satellite-stats-packet-collision-helper.h',
        'stats/satellite-stats-packet-drop-rate-helper.h',
        'stats/satellite-stats-packet-error-helper.h',
        'stats/satellite-stats-plt-helper.h',
        'stats/satellite-stats-queue-helper.h',
        'stats/satellite-stats-rbdc-request-helper.h',
        'stats/satellite-stats-resources-granted-helper.h',
        'stats/satellite-stats-satellite-queue-helper.h',
        'stats/satellite-stats-signalling-load-helper.h',
        'stats/satellite-stats-throughput-helper.h',
        'stats/satellite-stats-waveform-usage-helper.h',
        'stats/satellite-stats-window-load-helper.h',
        ]

    bld.add_pre_fun(compile_generator)

    if (bld.env['ENABLE_EXAMPLES']):
        bld.recurse('examples')

    # bld.ns3_python_bindings()

