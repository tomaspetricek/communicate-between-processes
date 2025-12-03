#ifndef THREADING_USE_PROMISE_HPP
#define THREADING_USE_PROMISE_HPP

#include <future>
#include <vector>

namespace net
{
    struct payload
    {
    };

    struct data_packet
    {
        int id;
        payload payload;
    };

    struct outgoing_packet
    {
        std::promise<bool> promise;
        payload payload;
    };

    class connection
    {
        std::promise<payload> promise;

    public:
        bool has_incoming_data() { return true; }

        data_packet incoming() { return data_packet(); }

        std::promise<payload> &get_promise(int id) { return promise; }

        bool has_outgoing_data() { return false; }

        outgoing_packet top_of_outgoing_queue() { return outgoing_packet(); }

        void send(const payload &) {}
    };

    using connection_set = std::vector<connection>;

    bool done(const connection_set &connections) { return false; }

    void process_connections(connection_set &connections)
    {
        while (!done(connections))
        {
            for (auto connection = connections.begin(); connection != connections.end();
                 ++connection)
            {
                if (connection->has_incoming_data())
                {
                    const auto data = connection->incoming();
                    auto &promise = connection->get_promise(data.id);
                    promise.set_value(data.payload);
                }
                if (connection->has_outgoing_data())
                {
                    auto data = connection->top_of_outgoing_queue();
                    connection->send(data.payload);
                    data.promise.set_value(true);
                }
            }
        }
    }
} // namespace net

#endif // THREADING_USE_PROMISE_HPP