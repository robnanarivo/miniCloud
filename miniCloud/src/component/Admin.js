import React from "react";
import {
  Form,
  FormInput,
  FormGroup,
  Button,
  Card,
  CardBody,
  CardTitle,
} from "shards-react";
import {
  Table,
  Modal,
  Tag,
} from "antd";
import { getServers, getDetailTable, getRaw, killNode, restartNode, getFrontEndTable } from "../fetcher";

const { Column, ColumnGroup } = Table;

class Admin extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      serverTable: [],
      rowColTable: [],
      frontEndTable: [],
      showingStatus: "main",
      currentAddress: null,
      isModalOpen: false,
      rawData: "<no content>"
    };

    this.handleClickFirst = this.handleClickFirst.bind(this);
    this.handleClickSecond = this.handleClickSecond.bind(this);
    this.showModal = this.showModal.bind(this);
    this.handleOk = this.handleOk.bind(this);
    this.handleCancel = this.handleCancel.bind(this);
  }
  showModal() {
    this.setState({isModalOpen: !this.state.isModalOpen})
  };
  handleOk() {
    this.setState({isModalOpen: false})
    //TODO call fetcher with email 
  };
  handleCancel() {
    this.setState({isModalOpen: false})
  };
  handleClickFirst(type, address) {
    this.setState({ showingStatus: type, currentAddress: address });
    getDetailTable(address).then((res) => {
      this.setState({ rowColTable: res });
    });
  }
  handleClickSecond(row, col) {
    // getDetailTable(row, col).then((res) => {
    //   this.setState({ rowColTable: res.results });
    // });
    getRaw(this.state.currentAddress, row, col).then((res) => {
      let t = res;
      this.setState({ rawData: t });
    });
    this.showModal();
  }
  componentDidMount() {
    getServers().then((res) => {
      let t = res;
      this.setState({ serverTable: t });
    });
    getFrontEndTable().then((res) => {
      let t = res;
      this.setState({ frontEndTable: t });
    });
  }

  render() {
    return (
      <div>
        <div style={{ width: "70vw", margin: "0 auto", marginTop: "5vh" }}>
          <h4>Services</h4>
          {this.state.serverTable && this.state.serverTable.length > 0 && (
            <Table
              dataSource={this.state.serverTable}
              pagination={{
                pageSizeOptions: [5, 10],
                defaultPageSize: 10,
                showQuickJumper: true,
              }}
            >
              <Column title="address" dataIndex="address" key="address" />
              <Column title="status" dataIndex="status" key="status" />
              <Column
                title="content"
                dataIndex="address"
                key="content"
                render={(text, row) => (
                  <Tag
                    color="#A1C7E0"
                    onClick={() =>
                      this.handleClickFirst("content", row.address)
                    }
                    key={"content " + row.address}
                    style={{ cursor: "pointer" }}
                  >
                    show content
                  </Tag>
                )}
              />
              <Column
                title="option"
                dataIndex="address"
                key="option"
                render={(text, row) => (
                  <>
                    <Tag
                      color="#A1C7E0"
                      key={"kill " + row.address}
                      style={{ cursor: "pointer" }}
                      onClick={()=>killNode(row.address)}
                    >
                      kill
                    </Tag>
                    <Tag
                      color="#A1C7E0"
                      key={"restart " + row.address}
                      style={{ cursor: "pointer" }}
                      onClick={()=>restartNode(row.address)}
                    >
                      restart
                    </Tag>
                  </>
                )}
              />
            </Table>
          )}
          <h4>Front End Services</h4>
          {this.state.frontEndTable && this.state.frontEndTable.length > 0 && (
            <Table
              dataSource={this.state.frontEndTable}
              pagination={{
                pageSizeOptions: [5, 10],
                defaultPageSize: 10,
                showQuickJumper: true,
              }}
            >
              <Column title="address" dataIndex="address" key="address" />
              <Column title="load" dataIndex="load" key="load" />
            </Table>
          )}
          {this.state.showingStatus != "main" && (
            <h4>
              {this.state.showingStatus} of address {this.state.currentAddress}
            </h4>
          )}
          {this.state.rowColTable && this.state.rowColTable.length > 0 && (
            <Table
              dataSource={this.state.rowColTable}
              pagination={{
                pageSizeOptions: [5, 10],
                defaultPageSize: 10,
                showQuickJumper: true,
              }}
            >
              <Column title="row" dataIndex="row" key="row" />
              <Column title="col" dataIndex="col" key="col" />
              <Column
                title="view"
                dataIndex="row"
                key="view"
                render={(text, row) => (
                  <Tag
                    color="#A1C7E0"
                    onClick={() => this.handleClickSecond(row.row, row.col)}
                    key={"content " + row.address}
                    style={{ cursor: "pointer" }}
                  >
                    view detail
                  </Tag>
                )}
              />
            </Table>
          )}

          <Modal
            title="raw data"
            open={this.state.isModalOpen}
            onOk={() => {
              this.handleOk();
            }}
            onCancel={() => {
                this.handleCancel();
              }}
          >
            <div>{this.state.rawData}</div>
            
          </Modal>
        </div>
      </div>
    );
  }
}

export default Admin;
