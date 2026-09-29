void Stub_Lock_Collector::record(u64 request_delay_ms) {
    std::lock_guard<std::mutex> lock(this->mutex);
}

Snapshot Stub_Lock_Collector::get_snapshot() {
    std::lock_guard<std::mutex> lock(this->mutex);
    return Single_Threaded::get_snapshot(); 
}
