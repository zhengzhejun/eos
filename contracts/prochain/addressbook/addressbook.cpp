//
// Created by 郑喆君 on 2018/6/20.
//

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/print.hpp>

using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;

class addressbook : public eosio::contract {
public:
    explicit addressbook(action_name self) : contract(self) {};

    void add(const account_name account, const string& name, uint64_t phone) {

        eosio::print("enter add, ", eosio::name{account});
        require_auth(account);

        eosio::print("contract owner is ", eosio::name{account});
        address_index addresses(_self, _self);

        auto itr = addresses.find(account);
        eosio_assert(itr == addresses.end(), "Address for acount already exist");

        addresses.emplace(account, [&]( auto& address ) {
            address.account = account;
            address.name = name;
            address.phone = phone;
        });
    }

    void update(const account_name account, const string& name, uint64_t phone) {
        eosio::print("enter update,", eosio::name{account});
        require_auth(account);

        address_index addresses(_self, _self);
        auto itr = addresses.find(account);
        eosio_assert(itr != addresses.end(), "Address for account not found");

        addresses.modify(itr, account, [&](auto& address) {
            address.account = account;
            address.name = name;
            address.phone = phone;
        });

    }

    void remove(const account_name account, const string& name, uint64_t phone) {
        eosio::print("enter remove,", eosio::name{account});
        address_index addresses(_self, _self);

        auto itr = addresses.find(account);
        eosio_assert(itr != addresses.end(), "Address for account not found");
        addresses.erase(itr);
    }

    void like(const account_name account) {
        eosio::print("enter like, ", eosio::name{account});
        address_index addresses(_self, _self);
        auto itr = addresses.find(account);
        eosio_assert(itr != addresses.end(), "Address for account not found");

        addresses.modify(itr, 0, [&](auto& address) {
            eosio::print("Liking: ", address.name.c_str(), "\n");
            address.liked++;
        });
    }

    void likebyphone(uint64_t phone) {
        eosio::print("enter like by phone,", phone);
        address_index addresses(_self, _self);
        auto phone_index = addresses.get_index<N(phone)>();
        auto itr = phone_index.lower_bound(phone);
        for(; itr != phone_index.end() && itr->phone == phone; itr++) {
            phone_index.modify(itr, 0, [&](auto& address) {
                eosio::print("liking:", address.name.c_str(), "\n");
                address.liked++;
            });
        }
    }



private:
    struct address {
        action_name account;
        string name;
        uint64_t phone;
        uint64_t liked;

        uint64_t primary_key() const { return account; }
        uint64_t get_phone() const { return phone; }
    };

    typedef eosio::multi_index<N(taddress), address, indexed_by<N(phone), const_mem_fun<address, uint64_t, &address::get_phone>>> address_index;
};

EOSIO_ABI(addressbook, (add)(update)(remove)(like)(likebyphone))

