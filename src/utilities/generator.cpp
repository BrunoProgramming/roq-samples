/* Copyright (c) 2017-2018, Hans Erik Thrane */

#include "utilities/generator.h"

#include <roq/logging.h>
#include <roq/stream.h>

#include <limits>

namespace examples {
namespace utilities {

const size_t MAX_COLUMNS = 40;
const char *TIME_FORMAT_FILE = "%Y%m%d %H:%M:%S";

const char *EXCHANGE = "CFFEX";
const uint32_t L1_TOPIC_ID = 100;  // CFFEX L1
const uint32_t L2_TOPIC_ID = 110;  // CFFEX L2

Generator::Generator(const std::string& path)
    : _csv_reader(path, MAX_COLUMNS) {}

Generator::~Generator() {
  LOG(INFO) << "Processed " << _message_id << " message(s)";
}

std::pair<bool, std::chrono::system_clock::time_point> Generator::fetch() {
  if (!_csv_reader.fetch())
    return std::make_pair(false, std::chrono::system_clock::time_point());
  ++_message_id;
  auto receive_time = _csv_reader.get_time_point(2, TIME_FORMAT_FILE);
  LOG_IF(FATAL, receive_time < _receive_time) << "Incorrect sequencing";
  _receive_time = receive_time;
  return std::make_pair(true, _receive_time);
}

void Generator::dispatch(roq::Strategy& strategy) {
  auto symbol = _csv_reader.get_string(0);
  auto exchange_time = _csv_reader.get_time_point(1, TIME_FORMAT_FILE);
  auto receive_time = _csv_reader.get_time_point(2, TIME_FORMAT_FILE);
  auto type = _csv_reader.get_integer(_csv_reader.length() - 1);  // last column is indicator for L1/L2
  switch (type) {
    case 0: return;  // L1 (don't process, for now)
    case 1: break;
    default: LOG(FATAL) << "Invalid type=" << type;
  }
  roq::MessageInfo message_info = {
    .source = "simulator",
    .source_create_time = receive_time,
    .client_receive_time = receive_time,
    .routing_latency = std::chrono::microseconds(0),
    .from_cache = false,
    .is_last = false,
  };
  strategy.on(roq::BatchBeginEvent { .message_info = message_info});
  roq::MarketByPrice market_by_price = {
    .exchange = EXCHANGE,
    .instrument = symbol.c_str(),
    .depth = {},
    .exchange_time = exchange_time,
    .channel = L2_TOPIC_ID,
  };
  for (auto i = 0; i < 5; ++i) {
    auto offset = 3 + (i * 4);
    roq::Layer& layer = market_by_price.depth[i];
    layer.ask_price = _csv_reader.get_number(offset + 0);
    layer.ask_quantity = _csv_reader.get_number(offset + 1);
    layer.bid_price = _csv_reader.get_number(offset + 2);
    layer.bid_quantity = _csv_reader.get_number(offset + 3);
  }
  VLOG(1) << market_by_price;
  strategy.on(roq::MarketByPriceEvent {
      .message_info = message_info,
      .market_by_price = market_by_price });
  roq::TradeSummary trade_summary = {
    .exchange = EXCHANGE,
    .instrument = symbol.c_str(),
    .price = _csv_reader.get_number(24),
    .volume = _csv_reader.get_number(34),
    .turnover = _csv_reader.get_number(33),
    .direction = roq::TradeDirection::Undefined,
    .exchange_time = exchange_time,
    .channel = L2_TOPIC_ID,
  };
  VLOG(1) << trade_summary;
  message_info.is_last = true;
  strategy.on(roq::TradeSummaryEvent {
      .message_info = message_info,
      .trade_summary = trade_summary });
  strategy.on(roq::BatchEndEvent { .message_info = message_info });
}

}  // namespace utilities
}  // namespace examples
