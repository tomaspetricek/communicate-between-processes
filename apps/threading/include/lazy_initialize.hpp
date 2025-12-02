#ifndef THREADING_LAZY_INITIALIZE_HPP
#define THREADING_LAZY_INITIALIZE_HPP

#include <mutex>

struct data_packet
{
};
struct connection_info
{
};
struct connection_handle
{
    void send_data(const data_packet &packet) {}
    data_packet receive_data() { return data_packet(); }
};
struct connection_manager
{
    static connection_handle open(const connection_info &info)
    {
        return connection_handle();
    }
};

class Z
{
    connection_info connection_details;
    connection_handle connection;
    std::once_flag connection_init_flag;

    void open_connection()
    {
        connection = connection_manager::open(connection_details);
    }

public:
    Z(const connection_info &info) : connection_details(info) {}

    void send_data(const data_packet &packet)
    {
        std::call_once(connection_init_flag, &Z::open_connection, this); // lazy initialization
        connection.send_data(packet);
    }

    data_packet receive_data()
    {
        std::call_once(connection_init_flag, &Z::open_connection, this); // lazy initialization
        return connection.receive_data();
    }
};

#endif // THREADING_LAZY_INITIALIZE_HPP