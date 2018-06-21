//
// Created by 郑喆君 on 2018/6/20.
//

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/print.hpp>
#include "addressbook.hpp"

using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;

class addressbook : public eosio::contract {
public:
    explicit addressbook(action_name self) : contract(self) {};

    //@abi action
    void add(const account_name account, const string& name, uint64_t phone, const move& m) {

        eosio::print("enter add, ", eosio::name{account});
        eosio::print("m is ", eosio::name{m.challenger}, " and ", eosio::name{m.host}, "\n");
//        require_auth(account);

        eosio::print("contract owner is ", eosio::name{_self});
        address_index addresses(_self, _self);

        auto itr = addresses.find(account);
        eosio_assert(itr == addresses.end(), "Address for acount already exist");

        addresses.emplace(account, [&]( auto& address ) {
            address.account = account;
            address.name = name;
            address.phone = phone;
        });
    }

    //@abi action
    void update(const account_name account, const string& name, uint64_t phone) {
        eosio::print("enter update,", eosio::name{account});
//        require_auth(account);

        address_index addresses(_self, _self);
        auto itr = addresses.find(account);
        eosio_assert(itr != addresses.end(), "Address for account not found");

        addresses.modify(itr, account, [&](auto& address) {
            address.account = account;
            address.name = name;
            address.phone = phone;
        });

    }

    //@abi action
    void remove(const account_name account, const string& name, uint64_t phone) {
        eosio::print("enter remove,", eosio::name{account});
        address_index addresses(_self, _self);

        auto itr = addresses.find(account);
        eosio_assert(itr != addresses.end(), "Address for account not found");
        addresses.erase(itr);
    }

    //@abi action
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

    //@abi action
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

    //@abi action
    void list(uint64_t phone) {
        eosio::print("enter list\n");
        address_index addresses(_self, _self);
        auto phone_index = addresses.get_index<N(phone)>();
        auto itr = phone_index.lower_bound(phone);
        for(; itr != phone_index.end(); itr++){
            eosio::print(itr->phone, "\n");
        }
    }



private:
    //@abi table taddress i64
    struct address {
        action_name account;
        string name;
        uint64_t phone;
        uint64_t liked;

        uint64_t primary_key() const { return account; }
        uint64_t get_phone() const { return phone; }

        EOSLIB_SERIALIZE(address, (account)(name)(phone)(liked));
    };

    typedef eosio::multi_index<N(taddress), address, indexed_by<N(phone), const_mem_fun<address, uint64_t, &address::get_phone>>> address_index;
};

EOSIO_ABI(addressbook, (add)(update)(remove)(like)(likebyphone)(list))

