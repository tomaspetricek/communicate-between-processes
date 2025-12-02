#ifndef THREADING_USE_SHARED_LOCK_HPP
#define THREADING_USE_SHARED_LOCK_HPP

#include <map>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <optional>

struct dns_entry
{
};

class dns_chache
{
    std::map<std::string, dns_entry> entries;
    mutable std::shared_mutex entry_mutex;

public:
    std::optional<dns_entry> find_entry(const std::string &domain) const
    {
        // for reading only - multiple threads can read
        std::shared_lock lock(entry_mutex);
        const auto it = entries.find(domain);

        if (it == entries.cend())
        {
            return std::nullopt;
        }
        return it->second;
    }

    void update_or_add_entry(const std::string &domain, const dns_entry &dns_details)
    {
        std::lock_guard lock(entry_mutex); // exlusive - only one thread may update
        entries[domain] = dns_details;
    }
};

#endif // THREADING_USE_SHARED_LOCK_HPP